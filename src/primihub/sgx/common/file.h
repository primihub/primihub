#ifndef SGX_COMMON_FILE_H_
#define SGX_COMMON_FILE_H_
#include <string>
class FileRWriter {
 public:
  explicit FileRWriter(const std::string &filename, const char *mode = "r");
  FileRWriter(const std::string &filename, const std::string &key, const char *mode = "r");
  ~FileRWriter();
  int read(void *ptr, size_t size);

  int write(const void *ptr, size_t size);
  int get_size(size_t *size);
  bool is_ok();
  int read_from_pos(void *ptr, size_t size, size_t offset) const;

 private:
  const std::string filename_;
  void *file_;
};
#endif  // SGX_COMMON_FILE_H_
