#include "ModeFactory.hpp"

#include "hexadecimal.hpp"

mode ModeFactory::benchmark() {
  mode r;
  r.function = ModeFunction::Benchmark;
  return r;
}

mode ModeFactory::zerobytes() {
  mode r;
  r.function = ModeFunction::ZeroBytes;
  return r;
}

mode ModeFactory::zeros() {
  return range(0, 0);
}

mode ModeFactory::all(int scoreMin) {
  mode r;
  r.function = ModeFunction::All;
  r.data1[0] = scoreMin;
  return r;
}

mode ModeFactory::allLeading() {
  mode r;
  r.function = ModeFunction::AllLeading;
  return r;
}

mode ModeFactory::allLeadingTrailing(const string strHex) {
  mode r;
  r.function = ModeFunction::AllLeadingTrailing;

  size_t len = strHex.size();

  if (len == 2) {
    r.data1[0] = hexValue(strHex[0]);
    r.data1[1] = hexValue(strHex[1]);
  } else if (len == 1) {
    r.data1[0] = hexValue(strHex[0]);
  }

  r.data2[0] = static_cast<cl_uchar>(len);

  return r;
}

mode ModeFactory::matchLeading(const string strHex) {
  mode r;
  r.function = ModeFunction::MatchLeading;

  auto len = strHex.size();

  for (size_t i = 0; i < len; ++i) {
    r.data1[i] = hexValueNoException(strHex[i]);
  }

  r.data2[0] = static_cast<cl_uchar>(len);

  return r;
}

mode ModeFactory::matching(const string strHex) {
  mode r;
  r.function = ModeFunction::Matching;

  fill(r.data1, r.data1 + sizeof(r.data1), cl_uchar(0));
  fill(r.data2, r.data2 + sizeof(r.data2), cl_uchar(0));

  auto index = 0;

  for (size_t i = 0; i < strHex.size(); i += 2) {
    const auto indexHi = hexValueNoException(strHex[i]);
    const auto indexLo = i + 1 < strHex.size() ? hexValueNoException(strHex[i + 1]) : string::npos;

    const auto valHi = (indexHi == string::npos) ? 0 : indexHi << 4;
    const auto valLo = (indexLo == string::npos) ? 0 : indexLo;

    const auto maskHi = (indexHi == string::npos) ? 0 : 0xF << 4;
    const auto maskLo = (indexLo == string::npos) ? 0 : 0xF;

    r.data1[index] = maskHi | maskLo;
    r.data2[index] = valHi | valLo;

    ++index;
  }

  return r;
}

mode ModeFactory::trailing(const char charLeading) {
  mode r;
  r.function = ModeFunction::Trailing;
  r.data1[0] = static_cast<cl_uchar>(hexValue(charLeading));
  return r;
}

mode ModeFactory::leading(const char charLeading) {
  mode r;
  r.function = ModeFunction::Leading;
  r.data1[0] = static_cast<cl_uchar>(hexValue(charLeading));
  return r;
}

mode ModeFactory::range(const cl_uchar min, const cl_uchar max) {
  mode r;
  r.function = ModeFunction::Range;
  r.data1[0] = min;
  r.data2[0] = max;
  return r;
}

mode ModeFactory::letters() {
  return range(10, 15);
}

mode ModeFactory::numbers() {
  return range(0, 9);
}

mode ModeFactory::leadingRange(const cl_uchar min, const cl_uchar max) {
  mode r;
  r.function = ModeFunction::LeadingRange;
  r.data1[0] = min;
  r.data2[0] = max;
  return r;
}

mode ModeFactory::mirror() {
  mode r;
  r.function = ModeFunction::Mirror;
  return r;
}

mode ModeFactory::doubles() {
  mode r;
  r.function = ModeFunction::Doubles;
  return r;
}
