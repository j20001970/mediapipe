#include "mediapipe/framework/resources.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "mediapipe/framework/port/status_macros.h"
#include "mediapipe/util/resource_util.h"

namespace mediapipe {

namespace {

class StringResource : public Resource {
 public:
  explicit StringResource(std::unique_ptr<std::string> s)
      : Resource(s->data(), s->size()), s_(std::move(s)) {};

 private:
  std::unique_ptr<std::string> s_;
};

// A Resource whose destructor does nothing.  Useful when some higher level is
// responsible for allocation/deletion of the actual data blocks.
class NoCleanupResource : public Resource {
 public:
  NoCleanupResource(const void* data, size_t length) : Resource(data, length) {}
};

class DefaultResources : public Resources {
 public:
  absl::Status ReadContents(absl::string_view resource_id, std::string& output,
                            const Options& options) const final {
    return GetResourceContents(std::string(resource_id), &output,
                               options.read_as_binary);
  }

  absl::StatusOr<std::unique_ptr<Resource>> Get(
      absl::string_view resource_id, const Options& options) const final {
    std::string contents;
    MP_RETURN_IF_ERROR(ReadContents(resource_id, contents, options));
    return MakeStringResource(std::move(contents));
  }
};

class ResourcesWithMapping : public Resources {
 public:
  explicit ResourcesWithMapping(
      std::unique_ptr<Resources> resources,
      absl::flat_hash_map<std::string, std::string> mapping)
      : resources_(std::move(resources)), mapping_(std::move(mapping)) {}

  absl::Status ReadContents(absl::string_view resource_id, std::string& output,
                            const Options& options) const final {
    auto iter = mapping_.find(resource_id);
    absl::string_view resolved_res_id;
    if (iter != mapping_.end()) {
      resolved_res_id = iter->second;
    } else {
      resolved_res_id = resource_id;
    }
    return resources_->ReadContents(resolved_res_id, output, options);
  }

  absl::StatusOr<std::unique_ptr<Resource>> Get(
      absl::string_view resource_id, const Options& options) const final {
    auto iter = mapping_.find(resource_id);
    absl::string_view resolved_res_id;
    if (iter != mapping_.end()) {
      resolved_res_id = iter->second;
    } else {
      resolved_res_id = resource_id;
    }
    return resources_->Get(resolved_res_id, options);
  }

 private:
  std::unique_ptr<Resources> resources_;
  absl::flat_hash_map<std::string, std::string> mapping_;
};

}  // namespace

std::unique_ptr<Resource> MakeStringResource(std::string&& s) {
  return std::make_unique<StringResource>(
      std::make_unique<std::string>(std::move(s)));
}

std::unique_ptr<Resource> MakeNoCleanupResource(const void* data,
                                                size_t length) {
  return std::make_unique<NoCleanupResource>(data, length);
}

std::unique_ptr<Resources> CreateDefaultResources() {
  return std::make_unique<DefaultResources>();
}

std::unique_ptr<Resources> CreateDefaultResourcesWithMapping(
    absl::flat_hash_map<std::string, std::string> mapping) {
  return CreateResourcesWithMapping(CreateDefaultResources(),
                                    std::move(mapping));
}

std::unique_ptr<Resources> CreateResourcesWithMapping(
    std::unique_ptr<Resources> resources,
    absl::flat_hash_map<std::string, std::string> mapping) {
  return std::make_unique<ResourcesWithMapping>(std::move(resources),
                                                std::move(mapping));
}

}  // namespace mediapipe
