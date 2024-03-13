#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <Windows.h>
#include <conio.h>
#include <thread>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <Wbemidl.h>
#include <string>
#include <future>
#include <omp.h>



using namespace std;
using namespace cv;

bool paused = false;

// Define region to search for the image
Rect region(960, 430, 20, 20); //940, 417, 20, 20

// Define image path and read the image
//string image_path_potato = "C:\\SB\\Images\\bigboss.png";
string image_path_potato = "C:\\SB\\Images\\javaw_eM8y2fmplN.png";
Mat template_img = imread(image_path_potato, IMREAD_COLOR);

// Buy RGB Values
int r_min = 240, r_max = 255;
int g_min = 240, g_max = 255;
int b_min = 130, b_max = 140;

// Click buy coords
int binsetX = 960; //960
int binsetY = 434; //434

// confirm buy coords
int consetX = 852;//852
int consetY = 410; //410

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
		KBDLLHOOKSTRUCT* pKBD = (KBDLLHOOKSTRUCT*)lParam;
		if (pKBD->vkCode == 'Q') {
			paused = !paused;
			if (paused) {
				cout << "PAUSED: Press 'q' again to resume." << endl;
				Beep(123, 200);
				Beep(90, 300);
			}
			else {
				cout << "WORKING..." << endl;
				Beep(723, 300);
			}
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}


void save_image(const Mat& img, const std::string& output_path) {
	imwrite(output_path, img);
}



// Button Functions
void click(int x, int y) {
	INPUT input[3] = { 0 };

	// Calculate the absolute x and y coordinates based on the screen resolution and scaling
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);
	int absolute_x = (x * 65535) / (screen_width - 1);
	int absolute_y = (y * 65535) / (screen_height - 1);

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
void keyboard_hook() {
	HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
	if (hKeyboardHook == NULL) {
		cout << "Failed to set keyboard hook." << endl;
		return;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hKeyboardHook);
}

cv::Mat hwnd2mat(const cv::Rect& region) {
	HWND desktop = GetDesktopWindow();
	HDC desktop_dc = GetDC(desktop);
	HDC capture_dc = CreateCompatibleDC(desktop_dc);

	HBITMAP capture_bitmap = CreateCompatibleBitmap(desktop_dc, region.width, region.height);
	SelectObject(capture_dc, capture_bitmap);

	BitBlt(capture_dc, 0, 0, region.width, region.height, desktop_dc, region.x, region.y, SRCCOPY | CAPTUREBLT);

	BITMAPINFOHEADER info_header = { 0 };
	info_header.biSize = sizeof(BITMAPINFOHEADER);
	info_header.biWidth = region.width;
	info_header.biHeight = -region.height; // Negative to indicate top-down bitmap
	info_header.biPlanes = 1;
	info_header.biBitCount = 24;
	info_header.biCompression = BI_RGB;

	cv::Mat mat(region.height, region.width, CV_8UC3);
	GetDIBits(capture_dc, capture_bitmap, 0, region.height, mat.data, (BITMAPINFO*)&info_header, DIB_RGB_COLORS);

	DeleteObject(capture_bitmap);
	DeleteDC(capture_dc);
	ReleaseDC(desktop, desktop_dc);

	return mat;
}

void thresholding(const Mat& res, double threshold, vector<Point>& loc) {
	Mat thresholded;
	cv::threshold(res, thresholded, threshold, 1.0, THRESH_BINARY);
	thresholded.convertTo(thresholded, CV_8U);

#pragma omp parallel for collapse(2) shared(thresholded, loc)
	for (int y = 0; y < thresholded.rows; y++) {
		for (int x = 0; x < thresholded.cols; x++) {
			if (thresholded.at<unsigned char>(y, x) > 0) {
#pragma omp critical
				{
					loc.push_back(Point(x, y));
				}
			}
		}
	}
}

// Function to process a region of the image
bool process_region(const Mat& img, int start_x, int end_x, int start_y, int end_y) {
	for (int x = start_x; x < end_x; x++) {
		for (int y = start_y; y < end_y; y++) {
			Vec3b bgr = img.at<Vec3b>(y, x);
			int b = bgr[0], g = bgr[1], r = bgr[2];

			if (r_min <= r && r <= r_max && g_min <= g && g <= g_max && b_min <= b && b <= b_max) {
				return true; // Change to return true if condition is met
			}
		}
	}
	return false; // Condition not met
}


bool multiscaleTemplateMatch(const Mat& img, const Mat& templ, Point& matchLoc, double matchThreshold = 0.2, double scaleStep = 0.9) {
	bool found = false;
	double maxVal = 0;

	for (double scale = 1.0; scale >= 0.5; scale *= scaleStep) {
		Size dsize = Size(templ.cols * scale, templ.rows * scale);
		Mat resizedTempl;
		resize(templ, resizedTempl, dsize);

		if (resizedTempl.rows > img.rows || resizedTempl.cols > img.cols) continue;

		Mat result;
		matchTemplate(img, resizedTempl, result, TM_CCOEFF_NORMED);
		minMaxLoc(result, NULL, &maxVal, NULL, &matchLoc);

		if (maxVal > matchThreshold) {
			found = true;
			matchLoc.x = int(matchLoc.x / scale);
			matchLoc.y = int(matchLoc.y / scale);
			break; // Exit early if a good match is found
		}
	}

	return found;
}


int main() {
	cout << "Welcome dismay" << endl;
	this_thread::sleep_for(chrono::seconds(2));
	cout << "Happy Flipping" << endl;

	thread kb_hook_thread(keyboard_hook);
	kb_hook_thread.detach();


	while (true) {
		// Sleep and do nothing
		if (paused) {
			this_thread::sleep_for(chrono::milliseconds(100));
			continue; // Repeat
		}

		// Take a screenshot of the specified region
		Mat img = hwnd2mat(region);

		if (img.empty()) {
			cerr << "Failed to capture image." << endl;
			return 0;
		}

		int num_threads = 4; // Adjust based on your CPU
		int step_x = img.cols / num_threads;
		vector<future<bool>> futures;

		for (int i = 0; i < num_threads; i++) {
			int start_x = i * step_x;
			int end_x = (i + 1) * step_x;
			if (i == num_threads - 1) {
				end_x = img.cols; // Ensure the last segment reaches the end of the image
			}
			futures.push_back(async(launch::async, process_region, cref(img), start_x, end_x, 0, img.rows));
		}

		// Check results from futures
		bool foundMatch = false;
		for (auto& fut : futures) {
			if (fut.get()) { // If any future returns true
				foundMatch = true;
				break; // Stop once we've found a match
			}
		}

		// Static function to buy and confirm purchase
		if (foundMatch) {
			// BIN Coord
			click(binsetX, binsetY);
			this_thread::sleep_for(chrono::milliseconds(3));
			// Confirm Buy Coord
			this_thread::sleep_for(chrono::milliseconds(3));
			click(consetX, consetY); // click buy
			this_thread::sleep_for(chrono::milliseconds(10));
			click(consetX, consetY);
			this_thread::sleep_for(chrono::milliseconds(10));
			click(consetX, consetY);
			this_thread::sleep_for(chrono::milliseconds(5));



			cout << "." << endl;
		}

		// Example debug output
		//string output_folder = "C:\\SB\\SavedImages\\";
		//static int saved_image_counter = 0; // Make counter static if you want it to persist across loop iterations

		//string output_path = output_folder + "capture_" + to_string(saved_image_counter++) + ".png";
		//save_image(img, output_path);

		//this_thread::sleep_for(chrono::milliseconds(100)); // Adjust delay as needed
	}
	return 0;
}

