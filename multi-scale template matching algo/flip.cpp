#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <Windows.h>
#include <conio.h>
#include <thread>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <Wbemidl.h>
#include <string>
#include <future>


using namespace std;
using namespace cv;

bool paused = false;

// Define region to search for the image
Rect region(960, 430, 20, 20); //940, 417, 20, 20

// Define image path and read the image
//string image_path_potato = "C:\\SB\\Images\\bigboss.png";
string image_path_potato = R"(C:\SB\Images\javaw_eM8y2fmplN.png)";
Mat template_img = imread(image_path_potato, IMREAD_COLOR);

// Buy RGB Values
int r_min = 240, r_max = 255;
int g_min = 240, g_max = 255;
int b_min = 130, b_max = 140;

// Click buy coords
int binsetX = 960; //960
int binsetY = 434; //434

// confirm buy coords
int consetX = 852; //852
int consetY = 410; //410

// Low Machine function to press Q and Beep Beeps
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// Check if nCode is HC_ACTION, indicating that the message contains valid information, and if the message is a key down event (WM_KEYDOWN).
	if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
	{
		// Cast lParam to a pointer of KBDLLHOOKSTRUCT to access the details of the keyboard event.
		const KBDLLHOOKSTRUCT* pKBD = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		if (pKBD->vkCode == 'Q')
		{
			paused = !paused;
			if (paused)
			{
				cout << "PAUSED: Press 'q' again to resume." << '\n';
				Beep(123, 200);
				Beep(90, 300);
			}
			else
			{
				cout << "WORKING..." << '\n';
				Beep(723, 300);
			}
		}
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// Write Image
void save_image(const Mat& img, const std::string& output_path)
{
	imwrite(output_path, img);
}


// Button Functions
void click(int x, int y)
{
	INPUT input[3] = {{0}};

	// Calculate the absolute x and y coordinates based on the screen resolution and scaling
	const int screen_width = GetSystemMetrics(SM_CXSCREEN);
	const int screen_height = GetSystemMetrics(SM_CYSCREEN);
	const int absolute_x = (x * 65535) / (screen_width - 1);
	const int absolute_y = (y * 65535) / (screen_height - 1);

	// Set up the input structures for moving the cursor
	input[0].type = INPUT_MOUSE;
	input[0].mi.dx = absolute_x;
	input[0].mi.dy = absolute_y;
	input[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

	// Set up the input structures for the left mouse button down
	input[1].type = INPUT_MOUSE;
	input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	// Set up the input structures for the left mouse button up
	input[2].type = INPUT_MOUSE;
	input[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

	// Send the inputs
	SendInput(3, input, sizeof(INPUT));
}

// Function to toggle pause state when 'q' is pressed
void keyboard_hook()
{
	const HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
	if (hKeyboardHook == nullptr)
	{
		cout << "Failed to set keyboard hook." << '\n';
		return;
	}

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// Unhook
	UnhookWindowsHookEx(hKeyboardHook);
}

// Convert a specified screen region to an OpenCV Mat object.
Mat hwnd2mat(const Rect& region)
{
	// Get a handle to the entire screen/desktop.
	const HWND desktop = GetDesktopWindow();
	// Obtain the device context (DC) for the desktop window, allowing drawing operations.
	const HDC desktop_dc = GetDC(desktop);
	// Create a compatible device context in memory, matching the desktop context.
	const HDC capture_dc = CreateCompatibleDC(desktop_dc);

	const HBITMAP capture_bitmap = CreateCompatibleBitmap(desktop_dc, region.width, region.height);
	SelectObject(capture_dc, capture_bitmap);
	// Perform a bit-block transfer
	BitBlt(capture_dc, 0, 0, region.width, region.height, desktop_dc, region.x, region.y, SRCCOPY | CAPTUREBLT);

	// Initialize a BITMAPINFOHEADER structure
	BITMAPINFOHEADER info_header = {0};
	info_header.biSize = sizeof(BITMAPINFOHEADER);
	info_header.biWidth = region.width;
	info_header.biHeight = -region.height; // Negative to indicate top-down bitmap
	info_header.biPlanes = 1;
	info_header.biBitCount = 24;
	info_header.biCompression = BI_RGB;

	// Create an OpenCV Mat object to hold the image data of the captured region.
	Mat mat(region.height, region.width, CV_8UC3);
	// Transfer the data from the captured bitmap into the OpenCV Mat object.
	GetDIBits(capture_dc, capture_bitmap, 0, region.height, mat.data, reinterpret_cast<BITMAPINFO*>(&info_header),
	          DIB_RGB_COLORS);

	// Clean up: release and delete the resources used for capturing and converting the screenshot.
	DeleteObject(capture_bitmap);
	DeleteDC(capture_dc);
	ReleaseDC(desktop, desktop_dc);

	return mat;
}

// // Applies thresholding to an image and collects the locations of all pixels that exceed the threshold.
void thresholding(const Mat& res, double threshold, vector<Point>& loc)
{
	Mat thresholded;
	// Apply a binary threshold to the image. All pixels with a value greater than 'threshold' are set to 1, others to 0.
	cv::threshold(res, thresholded, threshold, 1.0, THRESH_BINARY);
	// Convert the thresholded image to an 8-bit unsigned integer type for easier processing.
	thresholded.convertTo(thresholded, CV_8U);

	/* Use OpenMP to parallelize the loop, iterating over all pixels in 'thresholded'.
	'collapse(2)' combines the two nested for-loops into a single parallel loop.
	 'shared(thresholded, loc)' specifies that the threads share the original image and location vector.*/
#pragma omp parallel for collapse(2) shared(thresholded, loc) //omp parallel for collapse(2) shared(thresholded, loc)
	for (int y = 0; y < thresholded.rows; y++)
	{
		for (int x = 0; x < thresholded.cols; x++)
		{
			// Check if the current pixel value is greater than 0 (meaning it passed the threshold).
			if (thresholded.at<unsigned char>(y, x) > 0)
			{
				// 'omp critical' ensures that only one thread at a time can execute the code inside the block.
				// This is necessary to prevent concurrent modifications of the 'loc' vector.
#pragma omp critical
				{
					loc.emplace_back(x, y); // // Add the current pixel's coordinates to the 'loc' vector.
				}
			}
		}
	}
}

// Function to process a region of the image
bool process_region(const Mat& img, int start_x, int end_x, int start_y, int end_y)
{
	for (int x = start_x; x < end_x; x++)
	{
		for (int y = start_y; y < end_y; y++)
		{
			auto bgr = img.at<Vec3b>(y, x);
			const int b = bgr[0];
			const int g = bgr[1];
			const int r = bgr[2];

			if (r_min <= r && r <= r_max && g_min <= g && g <= g_max && b_min <= b && b <= b_max)
			{
				return true;
			}
		}
	}
	return false;
}


bool multiscale_template_match(const Mat& img, const Mat& templ, Point& matchLoc, double matchThreshold = 0.2,
                               double scaleStep = 0.9)
{
	bool found = false;
	double maxVal = 0;

	// Iterate over multiple scales, starting from 100% and decreasing.
	for (double scale = 1.0; scale >= 0.5; scale *= scaleStep)
	{
		// Calculate the new size based on the current scale.
		auto dsize = Size(templ.cols * scale, templ.rows * scale);
		Mat resizedTempl; // To hold the resized template image.
		resize(templ, resizedTempl, dsize); // Resize the template image.

		// Skip this scale if the resized template is larger than the source image.
		if (resizedTempl.rows > img.rows || resizedTempl.cols > img.cols) continue;

		Mat result; // Store

		// Perform template matching with the resized template.
		matchTemplate(img, resizedTempl, result, TM_CCOEFF_NORMED);
		// Find the maximum value and location in the result matrix.
		minMaxLoc(result, nullptr, &maxVal, nullptr, &matchLoc);

		// If the maximum value is greater than the threshold, a match has been found.
		if (maxVal > matchThreshold)
		{
			found = true;
			matchLoc.x = static_cast<int>(matchLoc.x / scale);
			matchLoc.y = static_cast<int>(matchLoc.y / scale);
			break; // Exit early if a good match is found
		}
	}

	return found;
}


bool check_pause()
{
	// Check for pause
	if (paused)
	{
		this_thread::sleep_for(chrono::milliseconds(100));
		return true; // Repeat
	}
	return false;
}

void buy_item(bool foundMatch)
{
	// Static function to buy and confirm purchase
	if (foundMatch)
	{
		// BIN Coord
		click(binsetX, binsetY);
		this_thread::sleep_for(chrono::milliseconds(3));
		// Confirm Buy Coord
		this_thread::sleep_for(chrono::milliseconds(5));
		click(consetX, consetY); // click buy
		this_thread::sleep_for(chrono::milliseconds(5));
		click(consetX, consetY);
		this_thread::sleep_for(chrono::milliseconds(5));
		click(consetX, consetY);


		cout << "." << '\n';
	}
}

// Unused debug code if need to use
void debug(Mat img)
{
	// Example debug output
	const string output_folder = R"(C:\SB\SavedImages\)";
	static int saved_image_counter = 0; // Make counter static if you want it to persist across loop iterations

	const string output_path = output_folder + "capture_" + to_string(saved_image_counter++) + ".png";
	save_image(img, output_path);

	this_thread::sleep_for(chrono::milliseconds(100)); // Adjust delay as needed
}

int main()
{
	cout << "Welcome dismay" << '\n';
	this_thread::sleep_for(chrono::seconds(2));
	cout << "Happy Flipping" << '\n';

	thread kb_hook_thread(keyboard_hook);
	kb_hook_thread.detach();


	while (true)
	{
		// Sleep and do nothing
		if (check_pause()) continue;

		// Take a screenshot of the specified region
		Mat img = hwnd2mat(region);

		// oopsie i'm an aquarius
		if (img.empty())
		{
			cerr << "Failed to capture image." << '\n';
			return 0;
		}

		constexpr int num_threads = 4; // Adjust based on your CPU
		const int step_x = img.cols / num_threads;
		// Calculate the width of each segment of the image to process separately.
		vector<future<bool>> futures; // Create a vector to hold futures

		// Divide the image into segments and process each segment in a separate asynchronous task.
		for (int i = 0; i < num_threads; i++)
		{
			int start_x = i * step_x;
			int end_x = (i + 1) * step_x;
			// Make sure the last segment covers the remainder of the image.
			if (i == num_threads - 1)
			{
				end_x = img.cols; // Ensure the last segment reaches the end of the image
			}
			// Launch an asynchronous task to process a segment of the image.
			futures.push_back(async(launch::async, process_region, cref(img), start_x, end_x, 0, img.rows));
		}

		// Check results from futures
		bool foundMatch = false;
		for (auto& fut : futures)
		{
			if (fut.get()) // Check if the future returned true, indicating a match was found.
			{
				// If any future returns true
				foundMatch = true;
				break; // Stop once we've found a match
			}
		}

		// If found, buy
		buy_item(foundMatch);


		//debug(img); // Optional: Debug Code
	}
}
