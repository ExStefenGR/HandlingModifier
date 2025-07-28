## Overview

This is the **GTA V Handling Modifier**, a graphical tool I developed to simplify the process of creating, editing, and fine-tuning vehicle handling data for Grand Theft Auto V. Originally built as a personal utility for my vehicle commission work, it provides a user-friendly interface for direct `handling.meta` file manipulation and real-time visualization of key vehicle physics.

If you're looking for an intuitive way to customize GTA V vehicle handling and understand its impact through visual feedback, this tool can help.

## Key Features

* **Handling File Management:**
    * **Automated Discovery:** Easily locate `handling.meta` files by Browse to your GTA V Vehicle data directory.
    * **Per-Vehicle Navigation:** Quickly jump between different vehicle `handling.meta` files for individual adjustments.
* **Comprehensive Handling Parameter Editing:**
    * **Core Physics:** Adjust fundamental vehicle properties such as:
        * **Top Speed** (`fInitialDriveMaxFlatVel`)
        * **Torque to Wheels** (`fInitialDriveForce` is calculated based on this and Mass)
        * **Mass** (`fMass`)
        * **Acceleration Offset** (fine-tunes `fInitialDriveForce` calculation)
        * **Wheel Drive Bias** (`fDriveBiasFront`)
    * **Damage Control:** Globally modify how vehicles take damage with attributes like `fEngineDamageMult`, `fCollisionDamageMult`, and `fWeaponDamageMult`.
    * **Brake Force Adjustment:** Automatically set `fBrakeForce` based on the vehicle's `fTractionCurveMax` value.
* **Interactive Traction Curve Visualization:**
    * **Real-time Plotting:** Observe how changes to traction-related parameters instantly impact the vehicle's tire grip curve. Parameters include:
        * `fTractionCurveMax`
        * `fTractionCurveMin`
        * `fTractionCurveLateral`
        * `fTractionSpringDeltaMax` (suspension influence on grip)
        * `fLowSpeedTractionLossMult`
        * `fCamberStiffnesss`
        * `fTractionBiasFront`
        * `fTractionLossMult` (surface grip factor)
    * **Visual Bias:** A dedicated bar chart clearly displays the distribution of front vs. rear drive bias.
* **Utility Functions:**
    * **Comment Cleaner:** Remove duplicate comments from `handling.meta` files for a cleaner structure.
    * **Low Drive Force Finder:** Identify vehicles with `fInitialDriveForce` below a specified threshold.
    * **Add Custom Comment:** Insert a personalized comment (e.g., for credits) into your `handling.meta` files.
* **Overlay Mode (Windows Only):** The application window can operate as a transparent, always-on-top overlay, allowing for quick adjustments while interacting with other applications.

## How to Use It

1.  **Run the Application:** Download the latest executable from the [Releases](https://github.com/ExStefenGR/HandlingModifier/releases/tag/2.5) page and run it. *(Remember to create a Release if this is a GitHub project!)*
2.  **Select GTA V Data Folder:** Upon launch, click the "Browse" button. You should point the tool to a folder that contains your `handling.meta` files.
    * **Recommended:** Point to your **`Vehicle/Data`** folder. The tool will recursively search within for `handling.meta` files.
    * The tool expects `handling.meta` files to be found in structures similar to `data\vehicle_folder\handling.meta`.
3.  **Choose an Operation:** Once a valid directory is selected and files are found, choose from the options presented in the interface to perform actions such as:
    * `Modify handling values (All cars)`: Change physics parameters for all vehicles (top speed, torque, mass).
    * `Delete Comments`: Remove all comments from `handling.meta` files.
    * `Find low drive force values`: Identify cars with very low acceleration.
    * `Modify damage values`: Adjust how cars take damage.
    * `Modify brakes based on fTractionCurveMax`: Set vehicle braking force based on traction.
    * `Add comment`: Add a custom comment to all files.
    * `Edit handling data for specific vehicle`: Select a particular vehicle folder (e.g., `18performante`) to edit its parameters.
    * `Exit`: Close the program via the GUI.
4.  **Adjust and Save:** Use the provided sliders and input fields to modify values. The traction curve plot will update instantly. Click "Save Changes" to apply your modifications to the respective `handling.meta` file.

## Todo / Future Plans

* Implement a dedicated **Aerodynamics Editor**.
* Enhance the **file extractor** to streamline `handling.meta` organization into `Structured VehicleFolder/handling.meta`.

## Important Notes

* **Always Back Up Your Files:** Before making any changes, it is crucial to create backups of your `handling.meta` files. This tool modifies game files directly, and while designed with safety in mind, unforeseen issues can occur.
* **GTA V Modding:** This tool involves modifying game data. Use it responsibly and understand any implications for your game installation.
* **Windows Compatibility:** The overlay functionality is dependent on Windows-specific API calls and will only work on Windows operating systems.

## Contributing

As an open-source project, contributions are welcome! If you find bugs, have suggestions for new features, or want to contribute code, please reach out.
