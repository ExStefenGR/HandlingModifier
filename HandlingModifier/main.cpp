// Define macros before includes
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Include other necessary headers
#define GLFW_EXPOSE_NATIVE_WIN32
#define _USE_MATH_DEFINES
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui.h"
#include <filesystem>
#include <glfw/glfw3.h>
#include <iostream>
#include <string>
#include <GLFW/glfw3native.h>
#include <WTypesbase.h>
#include <combaseapi.h>
#include <gl/GL.h>
#include <objbase.h>
#include <ShObjIdl_core.h>
#include <cstring>
#include <vector>
#include <stringapiset.h>
#include <WinNls.h>
#include <iosfwd>
#include <cmath>
#include "implot.h"
#include <__msvc_ostream.hpp>

// Import modularized utilities and modifiers
import utilities;
import modifiers;

int win_w = 1280;
int win_h = 900;

static void ShowTractionCurveVisualization(
	float fTractionCurveMax,
	float fTractionCurveMin,
	float fTractionCurveLateral,
	float fTractionSpringDeltaMax,
	float fLowSpeedTractionLossMult,
	float fCamberStiffnesss,
	float fTractionBiasFront,
	float fTractionLossMult
)
{
	static constexpr float PI = 3.14159265358979f;
	static constexpr float HALF_PI = PI * 0.5f;

	const int num_points = 500;
	std::vector<float> slip_angles(num_points);
	std::vector<float> traction_curve_effective(num_points);
	std::vector<float> traction_curve_base(num_points);

	float peakSlipAngle = fTractionCurveLateral * 0.5f;

	float effectiveLowSpeedMult = 1.0f - (1.0f - fLowSpeedTractionLossMult) * 0.2f;
	float effectiveTractionLossMult = 1.0f - (1.0f - fTractionLossMult) * 0.1f;

	for (int i = 0; i < num_points; ++i)
	{
		float t = static_cast<float>(i) / (num_points - 1);
		float slip = t * fTractionCurveLateral;
		slip_angles[i] = slip;

		float traction_at_slip_base = 0.0f;

		if (slip <= peakSlipAngle)
		{
			float ratio = (peakSlipAngle <= 0.001f) ? 1.0f : slip / peakSlipAngle;
			float sine_val = sinf(ratio * HALF_PI);
			traction_at_slip_base = fTractionCurveMin + (fTractionCurveMax - fTractionCurveMin) * (sine_val * sine_val);
		}
		else
		{
			float distPastPeak = slip - peakSlipAngle;
			float totalDistToFall = fTractionCurveLateral - peakSlipAngle;

			float ratio = (totalDistToFall > 0.001f) ? distPastPeak / totalDistToFall : 1.0f;

			float sine_val = sinf((1.0f - ratio) * HALF_PI);
			traction_at_slip_base = fTractionCurveMin + (fTractionCurveMax - fTractionCurveMin) * (sine_val * sine_val);
		}

		traction_curve_base[i] = traction_at_slip_base;
		float current_effective_traction = traction_at_slip_base * effectiveLowSpeedMult * effectiveTractionLossMult;
		traction_curve_effective[i] = current_effective_traction;
	}

	ImGui::BeginChild("Traction Curve Graph");

	if (ImPlot::BeginPlot("Traction Curve##Plot", ImVec2(-1, 300)))
	{
		ImPlot::SetupAxes("Slip Angle (Degrees)", "Traction Coefficient (G's)",
			ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);
		ImPlot::SetupFinish();

		ImPlot::SetNextLineStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(150, 150, 150, 128)), 0.8f);
		ImPlot::PlotLine("Base Traction", slip_angles.data(), traction_curve_base.data(), num_points);

		ImPlot::SetNextLineStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 150, 255, 255)), 2.0f);
		ImPlot::SetNextFillStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 100, 200, 80)));
		ImPlot::PlotShaded("##TractionShaded", slip_angles.data(), traction_curve_effective.data(), num_points);
		ImPlot::PlotLine("Effective Traction", slip_angles.data(), traction_curve_effective.data(), num_points);

		float effectivePeakTractionValue = fTractionCurveMax * effectiveLowSpeedMult * effectiveTractionLossMult;
		ImPlot::PlotScatter("Peak Grip", &peakSlipAngle, &effectivePeakTractionValue, 1);
		ImPlot::Annotation(peakSlipAngle, effectivePeakTractionValue, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec2(10, -25), false, "Peak Grip\n(%.1f deg, %.2f Gs)", peakSlipAngle, effectivePeakTractionValue);

		ImPlot::SetNextLineStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 100)), 1.0f);
		ImPlot::PlotInfLines("Effective Max Traction Ref", &effectivePeakTractionValue, 1);

		float effectiveMinTractionValue = fTractionCurveMin * effectiveLowSpeedMult * effectiveTractionLossMult;
		ImPlot::SetNextLineStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 255, 100)), 1.0f);
		ImPlot::PlotInfLines("Effective Min Traction Ref", &effectiveMinTractionValue, 1);

		if (fTractionSpringDeltaMax > 0.0f)
		{
			float max_spring_affected_traction = fTractionCurveMax * (1.0f + fTractionSpringDeltaMax);
			float min_spring_affected_traction = fTractionCurveMax * (1.0f - fTractionSpringDeltaMax);

			max_spring_affected_traction *= effectiveLowSpeedMult * effectiveTractionLossMult;
			min_spring_affected_traction *= effectiveLowSpeedMult * effectiveTractionLossMult;

			float half_width = fTractionCurveLateral * 0.02f;
			float spring_x_coords[2] = { peakSlipAngle - half_width, peakSlipAngle + half_width };
			float spring_y_upper[2] = { max_spring_affected_traction, max_spring_affected_traction };
			float spring_y_lower[2] = { min_spring_affected_traction, min_spring_affected_traction };

			ImPlot::SetNextFillStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 255, 0, 40)));
			ImPlot::PlotShaded("Traction Spring Delta Range", spring_x_coords, spring_y_upper, spring_y_lower, 2);

			ImPlot::Annotation(peakSlipAngle + half_width * 1.5, max_spring_affected_traction, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImVec2(0, -15), false, "Spring Boost");
			ImPlot::Annotation(peakSlipAngle + half_width * 1.5, min_spring_affected_traction, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImVec2(0, 15), false, "Spring Loss");
		}

		ImPlot::EndPlot();
	}

	if (ImPlot::BeginPlot("Traction Bias##Plot", ImVec2(-1, 150)))
	{
		ImPlot::SetupAxes("Axle", "Bias (%)", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxesLimits(-0.5, 1.5, 0.0, 1.0);
		ImPlot::SetupFinish();

		float frontBias = fTractionBiasFront;
		float rearBias = 1.0f - fTractionBiasFront;

		double x_pos[2] = { 0.0, 1.0 };
		double y_val[2] = { frontBias, rearBias };

		ImPlot::PlotBars("Front", &x_pos[0], &y_val[0], 1, 0.4f);
		ImPlot::PlotBars("Rear", &x_pos[1], &y_val[1], 1, 0.4f);

		ImPlot::Annotation(x_pos[0], y_val[0], ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec2(0, -25), true, "Front");
		ImPlot::Annotation(x_pos[0], y_val[0], ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec2(0, -5), true, "%.2f", y_val[0]);

		ImPlot::Annotation(x_pos[1], y_val[1], ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec2(0, -25), true, "Rear");
		ImPlot::Annotation(x_pos[1], y_val[1], ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec2(0, -5), true, "%.2f", y_val[1]);

		float zero_point = 0.5f;
		ImPlot::SetNextLineStyle(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 200, 0, 100)), 1.0f);
		ImPlot::PlotInfLines("Balanced Bias", &zero_point, 1);

		ImPlot::EndPlot();
	}

	ImGui::Text("Traction Parameters:");
	ImGui::Text("  fTractionCurveMax: %.2f G's", fTractionCurveMax);
	ImGui::Text("  fTractionCurveMin: %.2f G's", fTractionCurveMin);
	ImGui::Text("  fTractionCurveLateral: %.1f degrees (Full Slip Range)", fTractionCurveLateral);
	ImGui::Text("  fTractionSpringDeltaMax: %.2f (Suspension Influence)", fTractionSpringDeltaMax);
	ImGui::Text("  fLowSpeedTractionLossMult: %.2f (Low Speed Grip Factor)", fLowSpeedTractionLossMult);
	ImGui::Text("  fCamberStiffnesss: %.2f (Drift/Slide Modifier)", fCamberStiffnesss);
	ImGui::Text("  fTractionBiasFront: %.2f (Front) / %.2f (Rear)", fTractionBiasFront, 1.0f - fTractionBiasFront);
	ImGui::Text("  fTractionLossMult: %.2f (Surface Grip Factor)", fTractionLossMult);

	ImGui::EndChild();
}


static std::string get_executable_directory()
{
	return std::filesystem::current_path().string();
}

static std::string browseFolder() {
	// Initialize COM
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) {
		return "";
	}

	IFileDialog* pFileDialog = nullptr;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
	if (SUCCEEDED(hr)) {
		DWORD dwOptions;
		// Get the current options
		hr = pFileDialog->GetOptions(&dwOptions);
		if (SUCCEEDED(hr)) {
			// Modify options to pick folders
			pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
		}

		// Set the dialog title
		pFileDialog->SetTitle(L"Browse for Folder");

		// Show the dialog
		hr = pFileDialog->Show(NULL);
		if (SUCCEEDED(hr)) {
			IShellItem* pItem = nullptr;
			hr = pFileDialog->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFilePath = nullptr;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr)) {
					// Convert the PWSTR to a std::wstring
					std::wstring wPath(pszFilePath);
					CoTaskMemFree(pszFilePath);

					// Convert wstring to string using WideCharToMultiByte
					int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wPath.c_str(), -1, NULL, 0, NULL, NULL);
					if (bufferSize > 0) {
						std::vector<char> buffer(bufferSize);
						WideCharToMultiByte(CP_UTF8, 0, wPath.c_str(), -1, buffer.data(), bufferSize, NULL, NULL);
						std::string path(buffer.data());

						pItem->Release();
						pFileDialog->Release();
						CoUninitialize();
						return path;
					}
					else {
						// Handle conversion error
						std::cerr << "Failed to convert folder path to string." << std::endl;
					}
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}

	// Uninitialize COM
	CoUninitialize();
	return "";
}


static void processFoldersUI(modifiers& mod, utilities& util)
{
	static char directoryPath[256] = "";
	static int option = 0;
	static float topSpeed = 0.0f, torqueToWheels = 0.0f, mass = 0.0f;
	static float accelerationOffset = 0.0f;
	static int wheelDriveChoice = 0; // 0 = rear, 1 = front, 2 = 4WD, 3 = custom
	static float customDriveBiasValue = 0.5f;

	// fTraction params
	static float fTractionCurveMax = 2.0f;
	static float fTractionCurveMin = 1.0f;
	static float fTractionCurveLateral = 25.0f;
	static float fTractionSpringDeltaMax = 0.15f;
	static float fLowSpeedTractionLossMult = 1.0f;
	static float fCamberStiffnesss = 0.5f;
	static float fTractionBiasFront = 0.48f;
	static float fTractionLossMult = 1.0f;

	static std::vector<std::filesystem::path> handlingFiles;
	static int currentCarIndex = 0;
	static char creditComment[256] = "";
	static float engineDamageMult = 2.4f,
		collisionDamageMult = 2.0f,
		weaponDamageMult = 0.8f;
	static bool shouldLoadData = true;
	static bool directoryValidated = false;

	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.69f, 1.0f),
		"Handling Modifier v2.5-ALPHA by ExStefen");

	// Directory selection
	ImGui::InputText("Directory Path", directoryPath, sizeof(directoryPath));
	if (ImGui::Button("Browse")) {
		std::string selectedPath = browseFolder();
		if (!selectedPath.empty()) {
			selectedPath.copy(directoryPath, sizeof(directoryPath) - 1);
			directoryPath[sizeof(directoryPath) - 1] = '\0';
			mod.directoryPath = directoryPath;

			// Gather handling.meta
			std::filesystem::path directoryPathFS(mod.directoryPath);
			handlingFiles.clear();
			for (const auto& entry :
				std::filesystem::recursive_directory_iterator(directoryPathFS))
			{
				if (entry.path().filename() == "handling.meta") {
					handlingFiles.push_back(entry.path());
				}
			}
			directoryValidated = !handlingFiles.empty();
			currentCarIndex = 0;
			shouldLoadData = true;
		}
	}

	// Early return if not valid
	if (!directoryValidated) {
		ImGui::TextWrapped("Please select a valid directory with handling.meta files to proceed.");
		return;
	}

	// Options
	ImGui::Text("Choose an option:");
	ImGui::RadioButton("Modify handling values (All cars)", &option, 1);
	ImGui::RadioButton("Delete Comments", &option, 2);
	ImGui::RadioButton("Find low drive force values", &option, 3);
	ImGui::RadioButton("Modify damage values", &option, 4);
	ImGui::RadioButton("Modify brakes based on fTractionCurveMax", &option, 5);
	ImGui::RadioButton("Add comment", &option, 6);
	ImGui::RadioButton("Edit handling data for specific vehicle", &option, 7);
	ImGui::RadioButton("Exit", &option, 8);

	if (option == 1 && !handlingFiles.empty())
	{
		// We want to show the "handling UI" on the left and the traction curve on the right
		ImGui::Columns(2, "TwoColumnLayout", false);
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.5f);

		// === [LEFT COLUMN] ===
		ImGui::Text("Modify handling values for each vehicle:");
		ImGui::Text("Current Vehicle Folder: %s",
			handlingFiles[currentCarIndex].parent_path().filename().string().c_str());

		// Load data only once
		if (shouldLoadData) {
			float wheelDriveVal = 0.0f;
			mod.loadHandlingData(
				handlingFiles[currentCarIndex],
				topSpeed,
				torqueToWheels,
				mass,
				accelerationOffset,
				wheelDriveVal,
				fTractionCurveMax,
				fTractionCurveMin,
				fTractionCurveLateral,
				fTractionSpringDeltaMax,
				fLowSpeedTractionLossMult,
				fCamberStiffnesss,
				fTractionBiasFront,
				fTractionLossMult
			);
			// wheelDriveChoice logic
			wheelDriveChoice =
				(wheelDriveVal == 0.0f) ? 0 :
				(wheelDriveVal == 1.0f) ? 1 :
				(wheelDriveVal == 0.5f) ? 2 : 3;
			if (wheelDriveChoice == 3) {
				customDriveBiasValue = wheelDriveVal;
			}
			shouldLoadData = false;
		}

		// Show some inputs
		ImGui::InputFloat("Top Speed (km/h)", &topSpeed);
		ImGui::InputFloat("Torque to Wheels (Nm)", &torqueToWheels);
		ImGui::InputFloat("Mass (kg)", &mass);
		ImGui::InputFloat("Acceleration Offset", &accelerationOffset);

		ImGui::Text("Wheel Drive Type:");
		ImGui::RadioButton("Rear-Wheel Drive", &wheelDriveChoice, 0);
		ImGui::RadioButton("Front-Wheel Drive", &wheelDriveChoice, 1);
		ImGui::RadioButton("All-Wheel Drive", &wheelDriveChoice, 2);
		ImGui::RadioButton("Custom Drive Bias", &wheelDriveChoice, 3);

		if (wheelDriveChoice == 3) {
			ImGui::SliderFloat("Custom Drive Bias Value (0.01 - 1.0)",
				&customDriveBiasValue,
				0.01f, 1.0f);
		}

		if (ImGui::Button("Save Changes")) {
			float wheelDriveValToSave = (wheelDriveChoice == 3)
				? customDriveBiasValue
				: (wheelDriveChoice == 0
					? 0.0f
					: (wheelDriveChoice == 1 ? 1.0f : 0.5f));

			mod.modifyHandlingData(
				handlingFiles[currentCarIndex],
				topSpeed,
				torqueToWheels,
				mass,
				handlingFiles[currentCarIndex].parent_path().filename().string(),
				accelerationOffset,
				wheelDriveValToSave
			);
			ImGui::Text("Changes saved for current vehicle.");
		}

		if (ImGui::Button("Next Vehicle") &&
			currentCarIndex < handlingFiles.size() - 1)
		{
			currentCarIndex++;
			shouldLoadData = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Previous Vehicle") && currentCarIndex > 0) {
			currentCarIndex--;
			shouldLoadData = true;
		}

		// Switch to next column
		ImGui::NextColumn();

		// === [RIGHT COLUMN] ===

		ImGui::SliderFloat("fTractionCurveMax", &fTractionCurveMax, 0.0f, 5.0f);
		ImGui::SliderFloat("fTractionCurveMin", &fTractionCurveMin, 0.0f, 5.0f);
		ImGui::SliderFloat("fTractionCurveLat", &fTractionCurveLateral, 0.0f, 45.0f);
		ImGui::SliderFloat("fTractionSpringDeltaMax", &fTractionSpringDeltaMax, 0.0f, 2.0f);
		ImGui::SliderFloat("fLowSpeedTractionLossMult", &fLowSpeedTractionLossMult, 0.0f, 2.0f);
		ImGui::SliderFloat("fCamberStiffnesss", &fCamberStiffnesss, 0.0f, 1.0f);
		ImGui::SliderFloat("fTractionBiasFront", &fTractionBiasFront, 0.01f, 0.99f);
		ImGui::SliderFloat("fTractionLossMult", &fTractionLossMult, 0.0f, 2.0f);

		// Show traction curve 
		ShowTractionCurveVisualization(
			fTractionCurveMax,
			fTractionCurveMin,
			fTractionCurveLateral,
			fTractionSpringDeltaMax,
			fLowSpeedTractionLossMult,
			fCamberStiffnesss,
			fTractionBiasFront,
			fTractionLossMult
		);

		// End columns
		ImGui::Columns(1);
	}
	else if (option == 2) {
		for (const auto& filePath : handlingFiles) {
			mod.removeDuplicateComments(filePath);
		}
		ImGui::Text("Duplicate comments removed from all files.");
	}
	else if (option == 3) {
		util.findLowDriveForceValues(mod.directoryPath);
		ImGui::Text("Low drive force values scanned.");
	}
	else if (option == 4) {
		ImGui::InputFloat("Engine Damage Multiplier", &engineDamageMult);
		ImGui::InputFloat("Collision Damage Multiplier", &collisionDamageMult);
		ImGui::InputFloat("Weapon Damage Multiplier", &weaponDamageMult);

		if (ImGui::Button("Apply Damage Modifications")) {
			for (const auto& filePath : handlingFiles) {
				mod.modifyDamageValues(filePath, engineDamageMult, collisionDamageMult, weaponDamageMult);
			}
			ImGui::Text("Damage values modified for all files.");
		}
	}
	else if (option == 5) {
		if (ImGui::Button("Apply Brake Modifications")) {
			mod.modifyBrakeForceBasedOnTractionCurveMax(handlingFiles, mod.directoryPath);
			ImGui::Text("Brake force modified based on traction curve max for all files.");
		}
	}
	else if (option == 6) {
		ImGui::InputText("Credit Comment", creditComment, sizeof(creditComment));

		if (ImGui::Button("Add Credit to All Files")) {
			for (const auto& filePath : handlingFiles) {
				util.addCreditCommentToFile(filePath, creditComment);
			}
			ImGui::Text("Credit comment added to all files.");
		}
	}
	else if (option == 7) {
		static char specificFolderName[256] = "";
		static bool isSpecificFolder = false;

		ImGui::Text("Edit handling data for a specific vehicle:");
		ImGui::InputText("Specific Folder Name", specificFolderName, sizeof(specificFolderName));

		if (ImGui::Button("Target Specific Folder")) {
			isSpecificFolder = !isSpecificFolder;
			shouldLoadData = true;
		}

		if (isSpecificFolder && strlen(specificFolderName) > 0) {
			auto it = std::find_if(handlingFiles.begin(), handlingFiles.end(), [&](const std::filesystem::path& path) {
				return path.parent_path().filename().string() == specificFolderName;
				});

			if (it != handlingFiles.end()) {
				long int newCarIndex = std::distance(handlingFiles.begin(), it);
				if (newCarIndex != currentCarIndex) {
					currentCarIndex = newCarIndex;
					shouldLoadData = true;
				}
			}
			else {
				ImGui::Text("Folder not found: %s", specificFolderName);
			}
		}

		if (shouldLoadData) {
			float wheelDriveVal = 0.0f;
			mod.loadHandlingData(
				handlingFiles[currentCarIndex],
				topSpeed,
				torqueToWheels,
				mass,
				accelerationOffset,
				wheelDriveVal,
				fTractionCurveMax,
				fTractionCurveMin,
				fTractionCurveLateral,
				fTractionSpringDeltaMax,
				fLowSpeedTractionLossMult,
				fCamberStiffnesss,
				fTractionBiasFront,
				fTractionLossMult
			);
			wheelDriveChoice = (wheelDriveVal == 0.0f) ? 0 : (wheelDriveVal == 1.0f ? 1 : (wheelDriveVal == 0.5f ? 2 : 3));
			if (wheelDriveChoice == 3) {
				customDriveBiasValue = wheelDriveVal;
			}
			shouldLoadData = false;
		}

		ImGui::InputFloat("Top Speed (km/h)", &topSpeed);
		ImGui::InputFloat("Torque to Wheels (Nm)", &torqueToWheels);
		ImGui::InputFloat("Mass (kg)", &mass);
		ImGui::InputFloat("Acceleration Offset", &accelerationOffset);

		ImGui::Text("Wheel Drive Type:");
		ImGui::RadioButton("Rear-Wheel Drive", &wheelDriveChoice, 0);
		ImGui::RadioButton("Front-Wheel Drive", &wheelDriveChoice, 1);
		ImGui::RadioButton("All-Wheel Drive (4WD)", &wheelDriveChoice, 2);
		ImGui::RadioButton("Custom Drive Bias", &wheelDriveChoice, 3);

		if (wheelDriveChoice == 3) {
			ImGui::SliderFloat("Custom Drive Bias Value (0.01 - 1.0)", &customDriveBiasValue, 0.01f, 1.0f);
		}

		if (ImGui::Button("Save Changes")) {
			float wheelDriveValToSave = (wheelDriveChoice == 3) ? customDriveBiasValue : (wheelDriveChoice == 0 ? 0.0f : (wheelDriveChoice == 1 ? 1.0f : 0.5f));
			mod.modifyHandlingData(handlingFiles[currentCarIndex], topSpeed, torqueToWheels, mass, handlingFiles[currentCarIndex].parent_path().filename().string(), accelerationOffset, wheelDriveValToSave);
			ImGui::Text("Handling data updated for specific vehicle.");
		}
	}
	else if (option == 8) {
		glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
	}
}


int main(int, char**)
{
	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Create an undecorated, transparent, always-on-top window
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_FLOATING, GLFW_TRUE); // Always on top

	GLFWwindow* window = glfwCreateWindow(win_w, win_h, "Handling Modifier ALPHA", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Get native window handle
	HWND hwnd = glfwGetWin32Window(window);
	if (hwnd == NULL) {
		std::cerr << "Failed to get native window handle." << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Set window to be transparent and click-through initially
	LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); // Maintain full window opacity

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	ImPlot::CreateContext();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	// Set up the modifiers and utilities classes
	modifiers mod;
	utilities util;

	bool clickThrough = true;
	bool isDragging = false;
	POINT dragStartPoint = { 0, 0 };
	POINT windowStartPoint = { 0, 0 };

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Handle click-through toggle based on mouse position
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(hwnd, &cursorPos);
		bool cursorOverWindow =
			(cursorPos.x >= 0 && cursorPos.x <= win_w &&
				cursorPos.y >= 0 && cursorPos.y <= win_h);

		if (cursorOverWindow && clickThrough) {
			SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
			clickThrough = false;
		}
		else if (!cursorOverWindow && !clickThrough) {
			SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);
			clickThrough = true;
		}

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Make the ImGui window cover the entire GLFW window
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2((float)win_w, (float)win_h));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGui::Begin("Handling Modifiers", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground);

		ImGui::PopStyleVar(2); // Pop the style vars

		// Draggable region (custom title bar)
		const float titleBarHeight = 30.0f;
		ImGui::SetCursorPos(ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

		if (ImGui::InvisibleButton("DragRegion", ImVec2((float)win_w, titleBarHeight))) {
			// capture drag
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			int window_x, window_y;
			glfwGetWindowPos(window, &window_x, &window_y);
			glfwSetWindowPos(window, window_x + (int)delta.x, window_y + (int)delta.y);
		}

		ImGui::SetCursorPos(ImVec2(10, 5));
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.69f, 1.0f), "Drag Window from here.");

		ImGui::PopStyleColor(3); // Pop the color styles

		ImGui::Separator();

		// Now do your main UI
		processFoldersUI(mod, util);

		ImGui::End();

		// Render
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

app_shutdown:
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
