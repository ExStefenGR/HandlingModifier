export module utilities;

import <iostream>;
import <fstream>;
import <string>;
import <filesystem>;
import <unordered_set>;
import <limits>;
import <vector>;

export constexpr float LOW_DRIVE_FORCE_THRESHOLD = 0.1f;

export class utilities {
public:
	std::string trim(const std::string& str);
	void findLowDriveForceValues(const std::filesystem::path& directoryPath);
	void addCreditCommentToFile(const std::filesystem::path& filePath, const std::string& credit);

private:
	std::string line{};
	std::vector<std::filesystem::path> filesWithLowDriveForce{};
	bool inCommentBlock{};
	const std::string FILENAME = "handling.meta";
};

// Implementations

std::string utilities::trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first) {
		return "";
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

void utilities::findLowDriveForceValues(const std::filesystem::path& directoryPath) {
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath)) {
		if (entry.path().filename() == FILENAME) {
			std::ifstream inFile(entry.path());

			if (!inFile.is_open()) {
				std::cerr << "Error: Could not open file: " << entry.path() << std::endl;
				continue;
			}

			std::string line;
			while (std::getline(inFile, line)) {
				if (line.find("<fInitialDriveForce value=") != std::string::npos) {
					size_t pos = line.find("value=");
					if (pos != std::string::npos) {
						pos += 7;
						std::string valueStr;
						while (line[pos] != '\"') {
							valueStr += line[pos];
							pos++;
						}
						float driveForceValue = std::stof(valueStr);

						if (driveForceValue <= LOW_DRIVE_FORCE_THRESHOLD) {
							filesWithLowDriveForce.push_back(entry.path());
							inFile.close();
							break;  // Exit the loop after finding a file with low drive force
						}
					}
				}
			}

			inFile.close();
		}
	}

	if (!filesWithLowDriveForce.empty()) {
		std::cout << "Files with low drive force values (<= " << LOW_DRIVE_FORCE_THRESHOLD << "):" << std::endl;
		for (const auto& filePath : filesWithLowDriveForce) {
			std::cout << filePath << std::endl;
		}
	}
	else {
		std::cout << "No files with low drive force values (<= " << LOW_DRIVE_FORCE_THRESHOLD << ") found in the specified directory." << std::endl;
	}

	std::cout << "Press enter to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void utilities::addCreditCommentToFile(const std::filesystem::path& filePath, const std::string& credit) {
	try {
		std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::ate);

		if (!file.is_open()) {
			std::cerr << "Error: Could not open file for writing: " << filePath << std::endl;
			return;
		}

		bool commentAdded = false;

		file.seekg(0); // Move the file pointer to the beginning

		// Create a temporary file in the same directory as the original file
		std::filesystem::path tempFilePath = filePath.parent_path() / "temp_file.txt";

		// Ensuring the temporary file is unique
		int counter = 1;
		while (std::filesystem::exists(tempFilePath)) {
			tempFilePath = filePath.parent_path() / ("temp_file_" + std::to_string(counter) + ".txt");
			counter++;
		}

		std::ofstream tempFile(tempFilePath);

		std::string line;
		while (std::getline(file, line)) {
			if (!commentAdded && line.find("<HandlingData>") != std::string::npos) {
				tempFile << "<!-- " << credit << " -->" << std::endl;
				commentAdded = true;
			}
			tempFile << line << std::endl;
		}

		if (!commentAdded) {
			tempFile << "<!-- " << credit << " -->" << std::endl;
		}

		tempFile.close();
		file.close();

		// Replace original file with the modified temporary file
		std::filesystem::rename(tempFilePath, filePath);

		std::cout << "Credit comment added to file: " << filePath.filename() << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "An error occurred: " << e.what() << std::endl;
	}
}
