#pragma once
#include <Exception.hpp>
#include <IO/FileSystem.h>
#include <IO/IOUtil.h>
#include <fmt/format.h>
#include <vector>

namespace vksample {
	
	class FVDECLSPEC IOUtil {
	  public:
		static std::vector<char> readFileString(const std::string &filename, fragcore::IFileSystem *filesystem) {

			fragcore::Ref<fragcore::IO> ref =
				fragcore::Ref<fragcore::IO>(filesystem->openFile(filename.c_str(), fragcore::IO::IOMode::READ));
			std::vector<char> string = fragcore::IOUtil::readString<char>(ref);
			ref->close();
			return string;
		}

		static std::vector<char> readFileString(const std::string &filename) {
			return readFileString(filename, fragcore::FileSystem::getFileSystem());
		}

		template <typename T>
		static std::vector<T> readFileData(const std::string &filename, fragcore::IFileSystem *filesystem) {

			fragcore::Ref<fragcore::IO> ref =
				fragcore::Ref<fragcore::IO>(filesystem->openFile(filename.c_str(), fragcore::IO::IOMode::READ));
			std::vector<T> buffer = fragcore::IOUtil::readFileData<T>(ref);
			ref->close();
			return buffer;
		}
	};
} // namespace vksample
