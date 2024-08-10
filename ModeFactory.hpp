#ifndef HPP_MODEFACTORY
#define HPP_MODEFACTORY

#include <string>

#include "types.hpp"

class ModeFactory {
 private:
  ModeFactory();
  ModeFactory(ModeFactory& o);
  ~ModeFactory();

 public:
  static mode matching(const string strHex);
  static mode range(const cl_uchar min, const cl_uchar max);
  static mode leading(const char charLeading);
  static mode trailing(const string strHex);
  static mode leadingRange(const cl_uchar min, const cl_uchar max);
  static mode mirror();
  static mode all(int scoreMin);
  static mode allLeading();
  static mode matchLeading(const string strHex);
  static mode allLeadingTrailing(const string strHex);

  static mode benchmark();
  static mode zerobytes();
  static mode zeros();
  static mode letters();
  static mode numbers();
  static mode doubles();
};

#endif /* HPP_MODEFACTORY */
