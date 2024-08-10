#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <magic_enum.hpp>

#include "ArgParser.hpp"
#include "Dispatcher.hpp"
#include "ModeFactory.hpp"
#include "help.hpp"
#include "hexadecimal.hpp"
#include "sha3.hpp"

using namespace std;

string readFile(const char* const szFilename) {
  ifstream in(szFilename, ios::in | ios::binary);
  ostringstream contents;
  contents << in.rdbuf();
  return contents.str();
}

vector<cl_device_id> getAllDevices(cl_device_type deviceType = CL_DEVICE_TYPE_GPU) {
  vector<cl_device_id> vDevices;

  cl_uint platformIdCount = 0;
  clGetPlatformIDs(0, NULL, &platformIdCount);

  vector<cl_platform_id> platformIds(platformIdCount);
  clGetPlatformIDs(platformIdCount, platformIds.data(), NULL);

  for (auto it = platformIds.cbegin(); it != platformIds.cend(); ++it) {
    cl_uint countDevice;
    clGetDeviceIDs(*it, deviceType, 0, NULL, &countDevice);

    vector<cl_device_id> deviceIds(countDevice);
    clGetDeviceIDs(*it, deviceType, countDevice, deviceIds.data(), &countDevice);

    copy(deviceIds.begin(), deviceIds.end(), back_inserter(vDevices));
  }

  return vDevices;
}

template <typename T, typename U, typename V, typename W>
T clGetWrapper(U function, V param, W param2) {
  T t;
  function(param, param2, sizeof(t), &t, NULL);
  return t;
}

template <typename U, typename V, typename W>
string clGetWrapperString(U function, V param, W param2) {
  size_t len;
  function(param, param2, 0, NULL, &len);
  char* const szString = new char[len];
  function(param, param2, len, szString, NULL);
  string r(szString);
  delete[] szString;
  return r;
}

template <typename T, typename U, typename V, typename W>
vector<T> clGetWrapperVector(U function, V param, W param2) {
  size_t len;
  function(param, param2, 0, NULL, &len);
  len /= sizeof(T);
  vector<T> v;
  if (len > 0) {
    T* pArray = new T[len];
    function(param, param2, len * sizeof(T), pArray, NULL);
    for (size_t i = 0; i < len; ++i) {
      v.push_back(pArray[i]);
    }
    delete[] pArray;
  }
  return v;
}

vector<string> getBinaries(cl_program& clProgram) {
  vector<string> vReturn;
  auto vSizes = clGetWrapperVector<size_t>(clGetProgramInfo, clProgram, CL_PROGRAM_BINARY_SIZES);
  if (!vSizes.empty()) {
    unsigned char** pBuffers = new unsigned char*[vSizes.size()];
    for (size_t i = 0; i < vSizes.size(); ++i) {
      pBuffers[i] = new unsigned char[vSizes[i]];
    }

    clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, vSizes.size() * sizeof(unsigned char*), pBuffers, NULL);
    for (size_t i = 0; i < vSizes.size(); ++i) {
      string strData(reinterpret_cast<char*>(pBuffers[i]), vSizes[i]);
      vReturn.push_back(strData);
      delete[] pBuffers[i];
    }

    delete[] pBuffers;
  }

  return vReturn;
}

template <typename T>
bool printResult(const T& t, const cl_int& err) {
  cout << ((t == NULL) ? lexical_cast::write(err) : "OK") << endl;
  return t == NULL;
}

bool printResult(const cl_int err) {
  cout << ((err != CL_SUCCESS) ? lexical_cast::write(err) : "OK") << endl;
  return err != CL_SUCCESS;
}

string keccakDigest(const string data) {
  char digest[32];
  sha3(data.c_str(), data.size(), digest, 32);
  return string(digest, 32);
}

void trim(string& s) {
  const auto iLeft = s.find_first_not_of(" \t\r\n");
  if (iLeft != string::npos) {
    s.erase(0, iLeft);
  }

  const auto iRight = s.find_last_not_of(" \t\r\n");
  if (iRight != string::npos) {
    const auto count = s.length() - iRight - 1;
    s.erase(iRight + 1, count);
  }
}

string makePreprocessorInitHashExpression(const char* c3Addr, const string& c2AddrBinary, const char* c3ProxyHash) {
  random_device rd;
  mt19937_64 eng(rd());
  uniform_int_distribution<unsigned int> distr;  // C++ requires integer type: "C2338	note : char, signed char, unsigned char, int8_t, and uint8_t are not allowed"
  ethhash h = {{0}};

  h.b[0] = 0xff;
  for (int i = 0; i < 20; ++i) {
    h.b[i + 1] = c3Addr[i];
  }

  for (int i = 0; i < 16; ++i) {
    h.b[i + 21] = distr(eng);
  }
  for (int i = 16; i < 32; ++i) {
    h.b[i + 21] = c2AddrBinary[i - 12];
  }

  for (int i = 0; i < 32; ++i) {
    h.b[i + 53] = c3ProxyHash[i];
  }

  h.b[85] ^= 0x01;

  ostringstream oss;
  oss << hex;
  for (int i = 0; i < 25; ++i) {
    oss << "0x" << h.q[i];
    if (i + 1 != 25) {
      oss << ",";
    }
  }

  return oss.str();
}

const char* hexStringToConstChar(const string& hex) {
  size_t length = hex.length();
  char* charArray = new char[length / 2 + 1];
  for (size_t i = 0; i < length; i += 2) {
    string byteString = hex.substr(i, 2);
    char byte = (char)strtol(byteString.c_str(), nullptr, 16);
    charArray[i / 2] = byte;
  }
  charArray[length / 2] = '\0';
  return charArray;
}

int main(int argc, char** argv) {
  try {
    ArgParser argp(argc, argv);
    bool bHelp = false;
    bool bModeBenchmark = false;
    bool bModeZeroBytes = false;
    bool bModeZeros = false;
    bool bModeLetters = false;
    bool bModeNumbers = false;
    string strModeLeading;
    string strModeMatching;
    string strModeLeadingMatch;
    string strModeTrailing;
    string fileName;
    bool bModeLeadingRange = false;
    bool bModeRange = false;
    bool bModeMirror = false;
    bool bModeDoubles = false;
    bool allLeading = false;
    bool allLeadingTrailing = false;
    string leadingTrailing;
    int scoreAll = 0;
    int rangeMin = 0;
    int rangeMax = 0;
    uint scoreMin = 0;
    vector<size_t> vDeviceSkipIndex;
    size_t worksizeLocal = 128;
    size_t worksizeMax = 0;  // Will be automatically determined later if not overriden by user
    size_t size = 16777216;
    string c2Addr;
    string c3ProxyHash = "21c35dbe1b344a2488cf3321d6ce542f8e9f305544ff09e4993a62319a497c1f";
    string c3Addr = "00000000000029398fcE86f09FF8453c8D0Cd60D";
    string strInitCode;
    string strInitCodeFile;

    argp.addSwitch("ms", "min-score", scoreMin);
    argp.addSwitch("f", "file", fileName);

    argp.addSwitch("h", "help", bHelp);
    argp.addSwitch("b", "benchmark", bModeBenchmark);
    argp.addSwitch("z", "zero-bytes", bModeZeroBytes);
    argp.addSwitch("Z", "zeros", bModeZeros);
    argp.addSwitch("L", "letters", bModeLetters);
    argp.addSwitch("n", "numbers", bModeNumbers);
    argp.addSwitch("l", "leading", strModeLeading);
    argp.addSwitch("x", "matching", strModeMatching);
    argp.addSwitch("lr", "leading-range", bModeLeadingRange);
    argp.addSwitch("r", "range", bModeRange);
    argp.addSwitch("mr", "mirror", bModeMirror);
    argp.addSwitch("ld", "leading-doubles", bModeDoubles);
    argp.addSwitch("lx", "leading-match", strModeLeadingMatch);
    argp.addSwitch("lt", "leading-trailing", leadingTrailing);
    argp.addSwitch("t", "trailing", strModeTrailing);
    argp.addSwitch("a", "all", scoreAll);
    argp.addSwitch("al", "all-leading", allLeading);
    argp.addSwitch("alt", "all-leading-trailing", allLeadingTrailing);
    argp.addSwitch("m", "min", rangeMin);
    argp.addSwitch("M", "max", rangeMax);

    argp.addMultiSwitch('s', "skip", vDeviceSkipIndex);
    argp.addSwitch("w", "work", worksizeLocal);
    argp.addSwitch("W", "work-max", worksizeMax);
    argp.addSwitch("S", "size", size);

    argp.addSwitch("d", "deployer", c2Addr);
    argp.addSwitch("I", "init-code", strInitCode);
    argp.addSwitch("i", "init-code-file", strInitCodeFile);

    argp.addSwitch("c3", "c3-proxy-hash", c3ProxyHash);  // create2 PROXY_CHILD_BYTECODE hash
    argp.addSwitch("d3", "c3-deployer", c3Addr);         // create3 deployer address

    if (!argp.parse()) {
      cout << "error: bad arguments, try again :<" << endl;
      return 1;
    }

    if (bHelp) {
      cout << g_strHelp << endl;
      return 0;
    }

    // Parse hexadecimal values and/or read init code from file
    if (strInitCodeFile != "") {
      ifstream ifs(strInitCodeFile);
      if (!ifs.is_open()) {
        cout << "error: failed to open input file for init code" << endl;
        return 1;
      }
      strInitCode.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());
    }

    trim(strInitCode);
    const string c2AddrBinary = keccakDigest(parseHexadecimalBytes(c2Addr)).substr(12);
    const string strInitCodeDigest = keccakDigest(parseHexadecimalBytes(strInitCode));
    const char* c3Addr_chars = hexStringToConstChar(c3Addr);
    const char* c3ProxyHash_chars = hexStringToConstChar(c3ProxyHash);
    const string strPreprocessorInitStructure = makePreprocessorInitHashExpression(c3Addr_chars, c2AddrBinary, c3ProxyHash_chars);

    mode mode = ModeFactory::benchmark();
    if (bModeBenchmark) {
      mode = ModeFactory::benchmark();
    } else if (bModeZeroBytes) {
      mode = ModeFactory::zerobytes();
    } else if (bModeZeros) {
      mode = ModeFactory::zeros();
    } else if (bModeLetters) {
      mode = ModeFactory::letters();
    } else if (bModeNumbers) {
      mode = ModeFactory::numbers();
    } else if (!strModeLeading.empty()) {
      if (scoreMin == 0) scoreMin = 2;
      mode = ModeFactory::leading(strModeLeading.front());
    } else if (!strModeTrailing.empty()) {
      mode = ModeFactory::trailing(strModeTrailing);
    } else if (!strModeLeadingMatch.empty()) {
      if (scoreMin == 0) scoreMin = 2;
      mode = ModeFactory::matchLeading(strModeLeadingMatch);
    } else if (!strModeMatching.empty()) {
      mode = ModeFactory::matching(strModeMatching);
    } else if (bModeLeadingRange) {
      mode = ModeFactory::leadingRange(rangeMin, rangeMax);
    } else if (bModeRange) {
      mode = ModeFactory::range(rangeMin, rangeMax);
    } else if (bModeMirror) {
      mode = ModeFactory::mirror();
    } else if (bModeDoubles) {
      mode = ModeFactory::doubles();
    } else if (scoreAll > 0) {
      mode = ModeFactory::all(scoreAll);
    } else if (allLeading) {
      mode = ModeFactory::allLeading();
    } else if (!leadingTrailing.empty() || allLeadingTrailing) {
      mode = ModeFactory::allLeadingTrailing(leadingTrailing);
    } else {
      cout << g_strHelp << endl;
      return 0;
    }

    if (scoreMin == 0) scoreMin = 6;

    if (fileName.empty()) {
      fileName = string(magic_enum::enum_name(mode.function)) + "-" + to_string(chrono::steady_clock::now().time_since_epoch().count()) + ".txt";
    }

    const config cfg{fileName, scoreMin, std::chrono::steady_clock::now()};
    cout << "Output file: " << cfg.fileName << " | Min score:" << cfg.scoreMin << endl;

    vector<cl_device_id> vFoundDevices = getAllDevices();
    vector<cl_device_id> vDevices;
    map<cl_device_id, size_t> mDeviceIndex;

    vector<string> vDeviceBinary;
    vector<size_t> vDeviceBinarySize;
    cl_int errorCode;

    cout << "Devices:" << endl;
    for (size_t i = 0; i < vFoundDevices.size(); ++i) {
      // Ignore devices in skip index
      if (find(vDeviceSkipIndex.begin(), vDeviceSkipIndex.end(), i) != vDeviceSkipIndex.end()) {
        continue;
      }

      cl_device_id& deviceId = vFoundDevices[i];

      const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
      const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
      const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);

      cout << "  GPU" << i << ": " << strName << ", " << globalMemSize << " bytes available, " << computeUnits << " compute units" << endl;
      vDevices.push_back(vFoundDevices[i]);
      mDeviceIndex[vFoundDevices[i]] = i;
    }

    if (vDevices.empty()) {
      return 1;
    }

    cout << endl;
    cout << "Initializing OpenCL..." << endl;
    cout << "  Creating context..." << flush;
    auto clContext = clCreateContext(NULL, vDevices.size(), vDevices.data(), NULL, NULL, &errorCode);
    if (printResult(clContext, errorCode)) {
      return 1;
    }

    cl_program clProgram;
    if (vDeviceBinary.size() == vDevices.size()) {
      // Create program from binaries
      cout << "  Loading kernel from binary..." << flush;
      const unsigned char** pKernels = new const unsigned char*[vDevices.size()];
      for (size_t i = 0; i < vDeviceBinary.size(); ++i) {
        pKernels[i] = reinterpret_cast<const unsigned char*>(vDeviceBinary[i].data());
      }

      cl_int* pStatus = new cl_int[vDevices.size()];

      clProgram = clCreateProgramWithBinary(clContext, vDevices.size(), vDevices.data(), vDeviceBinarySize.data(), pKernels, pStatus, &errorCode);
      if (printResult(clProgram, errorCode)) {
        return 1;
      }
    } else {
      // Create a program from the kernel source
      cout << "  Compiling kernel..." << flush;
      const string strKeccak = readFile("keccak.cl");
      const string strVanity = readFile("eradicate2.cl");
      const char* szKernels[] = {strKeccak.c_str(), strVanity.c_str()};

      clProgram = clCreateProgramWithSource(clContext, sizeof(szKernels) / sizeof(char*), szKernels, NULL, &errorCode);
      if (printResult(clProgram, errorCode)) {
        return 1;
      }
    }

    // Build the program
    cout << "  Building program..." << flush;

    const string strBuildOptions = "-D ERADICATE2_MAX_SCORE=" + lexical_cast::write(ERADICATE2_MAX_SCORE) + " -D ERADICATE2_INITHASH=" + strPreprocessorInitStructure;
    if (printResult(clBuildProgram(clProgram, vDevices.size(), vDevices.data(), strBuildOptions.c_str(), NULL, NULL))) {
#ifdef ERADICATE2_DEBUG
      cout << endl;
      cout << "build log:" << endl;

      size_t sizeLog;
      clGetProgramBuildInfo(clProgram, vDevices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &sizeLog);
      char* const szLog = new char[sizeLog];
      clGetProgramBuildInfo(clProgram, vDevices[0], CL_PROGRAM_BUILD_LOG, sizeLog, szLog, NULL);

      cout << szLog << endl;
      delete[] szLog;
#endif
      return 1;
    }

    cout << endl;

    Dispatcher d(clContext, clProgram, worksizeMax == 0 ? size : worksizeMax, size, cfg);
    for (auto& i : vDevices) {
      d.addDevice(i, worksizeLocal, mDeviceIndex[i]);
    }

    d.run(mode);
    clReleaseContext(clContext);
    return 0;
  } catch (runtime_error& e) {
    cout << "runtime_error - " << e.what() << endl;
  } catch (...) {
    cout << "unknown exception occured" << endl;
  }

  return 1;
}
