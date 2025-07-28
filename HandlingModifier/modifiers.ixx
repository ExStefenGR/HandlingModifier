export module modifiers;

import <iostream>;
import <fstream>;
import <sstream>;
import <string>;
import <filesystem>;
import <unordered_set>;
import <limits>;
import <vector>;
import <regex>;
import utilities;

export class modifiers {
public:
    modifiers() = default;
    ~modifiers() = default;

    void processFolders(const std::filesystem::path& directoryPath, float topSpeed, float torqueToWheels, float mass, float accelerationOffset, float wheelDriveVal);
    std::string directoryPath{};

    bool removeDuplicateComments(const std::filesystem::path& filePath);
    void modifyDamageValues(const std::filesystem::path& filePath, float engineDamageMult, float collisionDamageMult, float weaponDamageMult);
    void modifyHandlingData(const std::filesystem::path& filePath, float topSpeed, float torqueToWheels, float mass, const std::string& folderName, float accelerationOffset, float wheelDriveVal);
    void modifyDriveForceBasedOnTorque(const std::vector<std::filesystem::path>& files, float torqueToWheels, float mass, const std::string& folderName);
    void modifyBrakeForce(const std::filesystem::path& filePath, float brakeForce, const std::string& folderName);
    void modifyBrakeForceBasedOnTractionCurveMax(const std::vector<std::filesystem::path>& files, const std::string& folderName);
    void editHandlingDataForFolder(const std::filesystem::path& directoryPath);
    void loadHandlingData(const std::filesystem::path& filePath,
		float& topSpeed,
		float& torqueToWheels,
		float& mass,
		float& accelerationOffset,
		float& wheelDriveVal,
		float& fTractionCurveMax,
		float& fTractionCurveMin,
		float& fTractionCurveLateral,
		float& fTractionSpringDeltaMax,
		float& fLowSpeedTractionLossMult,
		float& fCamberStiffnesss,
		float& fTractionBiasFront,
		float& fTractionLossMult);
    float extractTractionCurveMax(const std::filesystem::path& filePath);

private:
    std::string credit{};
    std::string folderName{};
    float topSpeed{}, torqueToWheels{}, mass{};
    float brakeForce{};
    float engineDamageMult = 2.4f;
    float collisionDamageMult = 2.0f;
    float weaponDamageMult = 0.8f;
    float tractionCurveMax{};
    bool exitRequested{};
    bool found{};
    int option{};
};


void modifiers::modifyDamageValues(const std::filesystem::path& filePath, float engineDamageMult, float collisionDamageMult, float weaponDamageMult)
{
	try
	{
		if (!std::filesystem::exists(filePath))
		{
			throw std::runtime_error("Error: File does not exist: " + filePath.string());
		}

		std::ifstream inFile(filePath);
		if (!inFile.is_open())
		{
			throw std::runtime_error("Error: Could not open file: " + filePath.string());
		}

		std::string tempFileName = filePath.string() + ".tmp";
		std::ofstream outFile(tempFileName);
		if (!outFile.is_open())
		{
			inFile.close();
			throw std::runtime_error("Error: Could not create temporary file: " + tempFileName);
		}

		std::string line;
		while (std::getline(inFile, line))
		{
			line = std::regex_replace(line, std::regex("<fEngineDamageMult value=\"(.+)\" />"), std::format("<fEngineDamageMult value=\"{:.5f}\" />", engineDamageMult));
			line = std::regex_replace(line, std::regex("<fCollisionDamageMult value=\"(.+)\" />"), std::format("<fCollisionDamageMult value=\"{:.5f}\" />", collisionDamageMult));
			line = std::regex_replace(line, std::regex("<fWeaponDamageMult value=\"(.+)\" />"), std::format("<fWeaponDamageMult value=\"{:.5f}\" />", weaponDamageMult));

			outFile << line << std::endl;
		}

		inFile.close();
		outFile.close();

		std::filesystem::rename(tempFileName, filePath);
		std::cout << "Modified: " << filePath << " for damage values. Press enter to return." << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "An error occurred: " << e.what() << std::endl;
	}
}

void modifiers::modifyHandlingData(const std::filesystem::path& filePath, float topSpeed, float torqueToWheels, float mass, const std::string& folderName, float accelerationOffset, float wheelDriveVal)
{
	try
	{
		if (!std::filesystem::exists(filePath))
		{
			throw std::runtime_error("Error: File does not exist: " + filePath.string());
		}

		std::ifstream inFile(filePath);
		if (!inFile.is_open())
		{
			throw std::runtime_error("Error: Could not open file: " + filePath.string());
		}

		std::string tempFileName = filePath.string() + ".tmp";
		std::ofstream outFile(tempFileName);
		if (!outFile.is_open())
		{
			inFile.close();
			throw std::runtime_error("Error: Could not create temporary file: " + tempFileName);
		}

		bool fDriveBiasFrontModified = false; // Flag to track if we changed fDriveBiasFront

		std::string line;
		while (std::getline(inFile, line))
		{
			// Modify the relevant values in the file
			line = std::regex_replace(line, std::regex("<fMass value=\"(.+)\" />"), std::format("<fMass value=\"{:.5f}\" />", mass));
			line = std::regex_replace(line, std::regex("<fInitialDriveMaxFlatVel value=\"(.+)\" />"), std::format("<fInitialDriveMaxFlatVel value=\"{:.5f}\" />", (topSpeed * 0.75f) / 0.9f));

			// Calculate and apply modified acceleration
			float acceleration = (torqueToWheels / mass) + accelerationOffset;
			line = std::regex_replace(line, std::regex("<fInitialDriveForce value=\"(.+)\" />"), std::format("<fInitialDriveForce value=\"{:.5f}\" />", acceleration));

			// Modify fDriveBiasFront if user provided a choice
			if (!fDriveBiasFrontModified && line.find("<fDriveBiasFront value=") != std::string::npos)
			{
				line = std::regex_replace(line, std::regex("<fDriveBiasFront value=\"(.+)\" />"), std::format("<fDriveBiasFront value=\"{:.5f}\" />", wheelDriveVal));
				fDriveBiasFrontModified = true;
			}

			outFile << line << std::endl;
		}

		inFile.close();
		outFile.close();

		// Rename the temporary file to the original file
		std::filesystem::rename(tempFileName, filePath);

		std::cout << "Modified: " << filePath << " in folder: " << folderName << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "An error occurred: " << e.what() << std::endl;
	}
}

void modifiers::loadHandlingData(const std::filesystem::path& filePath,
	float& topSpeed,
	float& torqueToWheels,
	float& mass,
	float& accelerationOffset,
	float& wheelDriveVal,
	float& fTractionCurveMax,
	float& fTractionCurveMin,
	float& fTractionCurveLateral,
	float& fTractionSpringDeltaMax,
	float& fLowSpeedTractionLossMult,
	float& fCamberStiffnesss,
	float& fTractionBiasFront,
	float& fTractionLossMult) {
	try {
		if (!std::filesystem::exists(filePath)) {
			throw std::runtime_error("Error: File does not exist: " + filePath.string());
		}

		std::ifstream inFile(filePath);
		if (!inFile.is_open()) {
			throw std::runtime_error("Error: Could not open file: " + filePath.string());
		}

		std::string line;
		while (std::getline(inFile, line)) {
			if (line.find("<fInitialDriveMaxFlatVel value=") != std::string::npos) {
				std::regex regex("<fInitialDriveMaxFlatVel value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					topSpeed = std::stof(match[1].str()) * 0.9f / 0.75f; // Convert back from modified value
				}
			}
			else if (line.find("<fInitialDriveForce value=") != std::string::npos) {
				std::regex regex("<fInitialDriveForce value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					float acceleration = std::stof(match[1].str());

					// Reverse the formula to get the torqueToWheels value
					torqueToWheels = (acceleration - accelerationOffset) * mass;
				}
			}
			else if (line.find("<fMass value=") != std::string::npos) {
				std::regex regex("<fMass value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					mass = std::stof(match[1].str());
				}
			}
			else if (line.find("<fDriveBiasFront value=") != std::string::npos) {
				std::regex regex("<fDriveBiasFront value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					wheelDriveVal = std::stof(match[1].str());
				}
			}
			// New lines for traction data:
			else if (line.find("<fTractionCurveMax value=") != std::string::npos)
			{
				std::regex regex("<fTractionCurveMax value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fTractionCurveMax = std::stof(match[1].str());
				}
			}
			else if (line.find("<fTractionCurveMin value=") != std::string::npos)
			{
				std::regex regex("<fTractionCurveMin value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fTractionCurveMin = std::stof(match[1].str());
				}
			}
			else if (line.find("<fTractionCurveLateral value=") != std::string::npos)
			{
				std::regex regex("<fTractionCurveLateral value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fTractionCurveLateral = std::stof(match[1].str());
				}
			}
			else if (line.find("<fTractionSpringDeltaMax value=") != std::string::npos)
			{
				std::regex regex("<fTractionSpringDeltaMax value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fTractionSpringDeltaMax = std::stof(match[1].str());
				}
			}
			else if (line.find("<fLowSpeedTractionLossMult value=") != std::string::npos)
			{
				std::regex regex("<fLowSpeedTractionLossMult value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fLowSpeedTractionLossMult = std::stof(match[1].str());
				}
			}
			else if (line.find("<fCamberStiffnesss value=") != std::string::npos)
			{
				std::regex regex("<fCamberStiffnesss value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fCamberStiffnesss = std::stof(match[1].str());
				}
			}
			else if (line.find("<fTractionBiasFront value=") != std::string::npos)
			{
				std::regex regex("<fTractionBiasFront value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fTractionBiasFront = std::stof(match[1].str());
				}
			}
			else if (line.find("<fTractionLossMult value=") != std::string::npos)
			{
				std::regex regex("<fTractionLossMult value=\"([0-9.]+)\" />");
				std::smatch match;
				if (std::regex_search(line, match, regex)) {
					fTractionLossMult = std::stof(match[1].str());
				}
			}
		}

		inFile.close();
	}
	catch (const std::exception& e) {
		std::cerr << "An error occurred while loading handling data: " << e.what() << std::endl;
	}
}

void modifiers::editHandlingDataForFolder(const std::filesystem::path& directoryPath)
{
	std::cout << "Enter the name of the folder containing handling.meta file: ";
	std::string folderName;
	std::cin.ignore();
	std::getline(std::cin, folderName);

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath))
	{
		if (entry.path().filename() == "handling.meta" && entry.path().parent_path().filename().string() == folderName)
		{
			float topSpeed, torqueToWheels, mass, accelerationOffset, wheelDriveVal;
			std::cout << "Enter top speed for car in km/h (folder: " << folderName << "): ";
			std::cin >> topSpeed;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "Enter torque to wheels for car in Nm (folder: " << folderName << "): ";
			std::cin >> torqueToWheels;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "Enter mass for car in kg (folder: " << folderName << "): ";
			std::cin >> mass;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "Enter acceleration offset (can be negative): ";
			std::cin >> accelerationOffset;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "Enter wheel drive bias value (0.0 - 1.0): ";
			std::cin >> wheelDriveVal;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			modifyHandlingData(entry.path(), topSpeed, torqueToWheels, mass, folderName, accelerationOffset, wheelDriveVal);
			break;
		}
	}

	std::cin.get();

	if (std::filesystem::recursive_directory_iterator(directoryPath).recursion_pending())
	{
		std::cerr << "No handling.meta file found in folder: " << folderName << std::endl;
	}
}

void modifiers::modifyDriveForceBasedOnTorque(const std::vector<std::filesystem::path>& files, float torqueToWheels, float mass, const std::string& folderName)
{
	for (const auto& filePath : files)
	{
		float accelerationOffset = 0.0f;  // Default value
		float wheelDriveVal = 0.5f;  // Default 4WD value for simplicity
		modifyHandlingData(filePath, 0.0f, torqueToWheels, mass, folderName, accelerationOffset, wheelDriveVal);
	}

	std::cin.get();

	if (files.empty())
	{
		std::cerr << "No handling.meta files specified." << std::endl;
	}
}


float modifiers::extractTractionCurveMax(const std::filesystem::path& filePath)
{

	std::ifstream inFile(filePath);

	if (!inFile.is_open())
	{
		std::cerr << "Error: Could not open file: " << filePath << std::endl;
		return 0.0f;
	}

	std::string line;
	while (std::getline(inFile, line))
	{

		if (line.find("<fTractionCurveMax value=") != std::string::npos)
		{
			size_t pos = line.find("value=");
			if (pos != std::string::npos)
			{
				pos += 7;
				std::string valueStr;
				while (line[pos] != '\"')
				{
					valueStr += line[pos];
					pos++;
				}

				return std::stof(valueStr);
			}
		}
	}

	return 0.0f;
}
void modifiers::modifyBrakeForce(const std::filesystem::path& filePath, float brakeForce, const std::string& folderName)
{
	std::ifstream inFile(filePath);

	if (!inFile.is_open())
	{
		std::cerr << "Error: Could not open file: " << filePath << std::endl;
		return;
	}

	std::string line;
	std::ofstream outFile(filePath.string() + ".tmp");

	while (std::getline(inFile, line))
	{
		if (line.find("<fBrakeForce value=\"") != std::string::npos)
		{
			outFile << "   <fBrakeForce value=\"" << brakeForce << "\" />" << std::endl;
		}
		else
		{
			outFile << line << std::endl;
		}
	}

	inFile.close();
	outFile.close();

	std::filesystem::remove(filePath);
	std::filesystem::rename(filePath.string() + ".tmp", filePath);

	std::cout << "Modified brake force in: " << filePath << " in folder: " << folderName << std::endl;;
}

void modifiers::modifyBrakeForceBasedOnTractionCurveMax(const std::vector<std::filesystem::path>& files, const std::string& folderName)
{
	for (const auto& filePath : files)
	{
		const float tractionCurveMax = extractTractionCurveMax(filePath);

		const float brakeForce = tractionCurveMax / 4.0f;

		modifyBrakeForce(filePath, brakeForce, folderName);
	}
}

bool modifiers::removeDuplicateComments(const std::filesystem::path& filePath)
{
	std::ifstream inFile(filePath);
	if (!inFile.is_open())
	{
		std::cerr << "Error: Could not open file: " << filePath << std::endl;
		return false;
	}

	std::string modifiedContents;
	std::string currentCommentBlock;

	std::string line;
	while (std::getline(inFile, line))
	{
		if (line.find("<!--") != std::string::npos)
		{
			currentCommentBlock = line;
		}
		else if (line.find("-->") != std::string::npos)
		{
			if (currentCommentBlock.empty())
			{
				continue; // Skip empty comment blocks
			}

			modifiedContents += currentCommentBlock + '\n';
			currentCommentBlock.clear(); // Clear current comment block
		}
		else
		{
			modifiedContents += line + '\n';
		}
	}

	inFile.close();

	std::ofstream outFile(filePath);
	if (!outFile.is_open())
	{
		std::cerr << "Error: Could not open file for writing: " << filePath << std::endl;
		return false;
	}

	outFile << modifiedContents;
	outFile.close();

	return true;
}


void modifiers::processFolders(const std::filesystem::path& directoryPath, float topSpeed, float torqueToWheels, float mass, float accelerationOffset, float wheelDriveVal)
{
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath))
	{
		if (entry.path().filename() == "handling.meta")
		{
			std::string folderName = entry.path().parent_path().filename().string();
			modifyHandlingData(entry.path(), topSpeed, torqueToWheels, mass, folderName, accelerationOffset, wheelDriveVal);
		}
	}
}