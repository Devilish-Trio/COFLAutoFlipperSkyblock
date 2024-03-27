// Default Template Matching with OpenCV

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
#include <omp.h> // Include the OpenMP header
#include <mutex>


using namespace std;
using namespace cv;

bool paused = false;

static Mat lastCapturedMat;
static std::chrono::steady_clock::time_point lastCaptureTime;
static std::mutex captureMutex;
static constexpr long long captureIntervalMs = 100; // Minimum interval between captures in milliseconds

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

// Low level keyboard hook to press Q and do da Beep Beeps
LRESULT CALLBACK KeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam)
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
void click(const int x, const int y)
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
// Static variables for caching
Mat hwnd2mat(const Rect& region)
{
	std::lock_guard<std::mutex> lock(captureMutex); // Ensure thread safety

	// Check if we should update the capture
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCaptureTime).count();

	if (elapsed < captureIntervalMs && !lastCapturedMat.empty())
	{
		// Return cached image if interval hasn't passed
		return lastCapturedMat.clone(); // Clone to ensure caller gets a fresh Mat they can modify
	}

	// Initialize variables for capture process
	HWND desktop = GetDesktopWindow();
	HDC desktop_dc = GetDC(desktop);
	HDC capture_dc = CreateCompatibleDC(desktop_dc);
	HBITMAP capture_bitmap = CreateCompatibleBitmap(desktop_dc, region.width, region.height);

	SelectObject(capture_dc, capture_bitmap);
	BitBlt(capture_dc, 0, 0, region.width, region.height, desktop_dc, region.x, region.y, SRCCOPY | CAPTUREBLT);

	BITMAPINFOHEADER info_header = {0};
	info_header.biSize = sizeof(BITMAPINFOHEADER);
	info_header.biWidth = region.width;
	info_header.biHeight = -region.height; // Negative to indicate top-down bitmap
	info_header.biPlanes = 1;
	info_header.biBitCount = 24;
	info_header.biCompression = BI_RGB;

	Mat mat(region.height, region.width, CV_8UC3);
	GetDIBits(capture_dc, capture_bitmap, 0, region.height, mat.data, reinterpret_cast<BITMAPINFO*>(&info_header),
	          DIB_RGB_COLORS);

	// Update the cache with the new capture
	lastCapturedMat = mat.clone(); // Clone to ensure cache stores a fresh copy
	lastCaptureTime = now; // Update capture time

	// Clean up resources used for this capture
	DeleteObject(capture_bitmap);
	DeleteDC(capture_dc);
	ReleaseDC(desktop, desktop_dc);

	return mat;
}


// Process Color
bool checkForSpecifiedColor(const Mat& img, const int r_min, const int r_max, const int g_min, const int g_max,
                            const int b_min, const int b_max)
{
	bool found = false;

#pragma omp parallel for collapse(2) // Parallelize both loops
	for (int y = 0; y < img.rows && !found; ++y)
	{
		for (int x = 0; x < img.cols && !found; ++x)
		{
			auto pixelColor = img.at<Vec3b>(y, x);
			int b = pixelColor[0];
			int g = pixelColor[1];
			int r = pixelColor[2];

			if (r >= r_min && r <= r_max && g >= g_min && g <= g_max && b >= b_min && b <= b_max)
			{
#pragma omp critical // Ensure thread safety when writing to the shared variable
				found = true;
			}
		}
	}
	return found;
}

// direct pixel comparison algo
bool direct_pixel_comparison_with_scale(const Mat& img, const Mat& originalTemplate,
                                        const double similarityThreshold = 0.99, const double scaleStep = 0.9,
                                        const double minScale = 0.5, const double maxScale = 1.5)
{
	bool foundMatch = false;
	double bestSimilarity = 0.0;

	for (double scale = minScale; scale <= maxScale; scale *= scaleStep)
	{
		// Calculate new dimensions based on the current scale to ensure they are valid
		Size newSize(static_cast<int>(originalTemplate.cols * scale), static_cast<int>(originalTemplate.rows * scale));

		// Skip this scale if the new size is invalid or if the resized template would be larger than the source image
		if (newSize.width <= 0 || newSize.height <= 0 || newSize.height > img.rows || newSize.width > img.cols)
		{
			continue;
		}

		// Resize the template according to the current scale
		Mat templ;
		resize(originalTemplate, templ, newSize, 0, 0, INTER_LINEAR);

		int matchCount = 0;
		int totalPixels = templ.rows * templ.cols;

		// Perform direct pixel comparison at the current scale
		for (int y = 0; y < templ.rows; ++y)
		{
			for (int x = 0; x < templ.cols; ++x)
			{
				// Assume both images are properly aligned; adjust indices if necessary
				auto pixelTempl = templ.at<Vec3b>(y, x);
				auto pixelImg = img.at<Vec3b>(y, x); // Direct indexing may need adjustment based on alignment

				if (pixelTempl == pixelImg)
				{
					matchCount++;
				}
			}
		}

		double similarity = static_cast<double>(matchCount) / totalPixels;
		bestSimilarity = max(similarity, bestSimilarity);

		// Check if the current similarity meets the threshold
		if (similarity >= similarityThreshold)
		{
			foundMatch = true;
			break;
		}
	}

	// For debugging or to adjust the similarity threshold dynamically
	//cout << "Best similarity: " << bestSimilarity << '\n';

	return foundMatch;
}

// Check for pause
bool check_pause()
{
	if (paused)
	{
		this_thread::sleep_for(chrono::milliseconds(100));
		return true; // Repeat
	}
	return false;
}

// Buy item
void buy_item(const bool foundMatch)
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

// Unused debug code 
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
		if (check_pause()) continue;

		Mat img = hwnd2mat(region);
		if (img.empty())
		{
			cerr << "Failed to capture image." << '\n';
			return 0;
		}

		bool colorPresent = checkForSpecifiedColor(img, r_min, r_max, g_min, g_max, b_min, b_max);
		if (colorPresent)
		{
			buy_item(true);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
