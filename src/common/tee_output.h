#ifndef TEE_OUTPUT_H
#define TEE_OUTPUT_H

#include <list>
#include <memory>
#include <string_view>

namespace logging
{
  namespace detail {
    class CapturingRdbuf;
    class FileOutput;
  }  // namespace detail

  class TeeOutput
  {
  public:
    explicit TeeOutput(std::string_view filename);
    TeeOutput(const TeeOutput&) = delete;
    TeeOutput& operator=(const TeeOutput&) = delete;
    ~TeeOutput();

  private:
    std::unique_ptr<detail::FileOutput> file_output;
    std::list<std::unique_ptr<detail::CapturingRdbuf>> rdbufs;
  };

} // namespace logging

#endif /* TEE_OUTPUT_H */
