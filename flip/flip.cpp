#include <iostream>
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include <conio.h>
#include <thread>
#include <chrono>
#include <mutex>

#include <vector>
#include <opencv2/imgproc.hpp>


using namespace std;
using namespace cv;

bool paused = false;
static Mat lastCapturedMat;
static std::chrono::steady_clock::time_point lastCaptureTime;
static std::mutex captureMutex;
static constexpr long long captureIntervalMs = 100;

Rect region(946, 456, 20, 20);
string image_path_potato = R"(C:\SB\Images\javaw_eM8y2fmplN.png)";
Mat template_img = imread(image_path_potato, IMREAD_COLOR);
Mat potato_img;
int r_min = 240, r_max = 255;
int g_min = 240, g_max = 255;
int b_min = 130, b_max = 140;
int binsetX = 959;
int binsetY = 450;
int consetX = 897;
int consetY = 459;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        const KBDLLHOOKSTRUCT* pKBD = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (pKBD->vkCode == 'Q') {
            paused = !paused;
            if (paused) {
                cout << "PAUSED: Press 'q' again to resume." << '\n';
                Beep(123, 200);
                Beep(90, 300);
            }
            else {
                cout << "WORKING..." << '\n';
                Beep(723, 300);
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void save_image(const Mat& img, const std::string& output_path) {
    imwrite(output_path, img);
}

void click(int x, int y) {
    static const int screen_width = GetSystemMetrics(SM_CXSCREEN);
    static const int screen_height = GetSystemMetrics(SM_CYSCREEN);

    INPUT input[3] = { {0} };

    const int absolute_x = (x * 65535) / (screen_width - 1);
    const int absolute_y = (y * 65535) / (screen_height - 1);

    input[0].type = INPUT_MOUSE;
    input[0].mi.dx = absolute_x;
    input[0].mi.dy = absolute_y;
    input[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    input[2].type = INPUT_MOUSE;
    input[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(3, input, sizeof(INPUT));
}

void keyboard_hook() {
    const HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
    if (hKeyboardHook == nullptr) {
        cout << "Failed to set keyboard hook." << '\n';
        return;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyboardHook);
}

Mat hwnd2mat(const Rect& region) {
    std::lock_guard<std::mutex> lock(captureMutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCaptureTime).count();

    if (elapsed < captureIntervalMs && !lastCapturedMat.empty()) {
        return lastCapturedMat.clone();
    }

    HWND desktop = GetDesktopWindow();
    HDC desktop_dc = GetDC(desktop);
    HDC capture_dc = CreateCompatibleDC(desktop_dc);
    HBITMAP capture_bitmap = CreateCompatibleBitmap(desktop_dc, region.width, region.height);

    SelectObject(capture_dc, capture_bitmap);
    BitBlt(capture_dc, 0, 0, region.width, region.height, desktop_dc, region.x, region.y, SRCCOPY | CAPTUREBLT);

    BITMAPINFOHEADER info_header = { 0 };
    info_header.biSize = sizeof(BITMAPINFOHEADER);
    info_header.biWidth = region.width;
    info_header.biHeight = -region.height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = BI_RGB;

    Mat mat(region.height, region.width, CV_8UC3);
    GetDIBits(capture_dc, capture_bitmap, 0, region.height, mat.data, reinterpret_cast<BITMAPINFO*>(&info_header), DIB_RGB_COLORS);

    lastCapturedMat = mat.clone();
    lastCaptureTime = now;

    DeleteObject(capture_bitmap);
    DeleteDC(capture_dc);
    ReleaseDC(desktop, desktop_dc);

    return mat;
}

// Non-Maximum Suppression
void nonMaximumSuppression(const Mat& matchResult, std::vector<Point>& matchLocations, double threshold)
{
	double minVal, maxVal;
	Point minLoc, maxLoc;

	Mat result = matchResult.clone();
	while (true)
	{
		minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
		if (maxVal < threshold)
		{
			break;
		}

		matchLocations.push_back(maxLoc);
		floodFill(result, maxLoc, Scalar(0), nullptr, Scalar(0.1), Scalar(0.1));
	}
}

bool detectPotato(const Mat& img, const Mat& templ, double matchThreshold = 0.95)
{
	// Convert images to grayscale
	Mat grayImg, grayTempl;
	cvtColor(img, grayImg, COLOR_BGR2GRAY);
	cvtColor(templ, grayTempl, COLOR_BGR2GRAY);

	// Apply GaussianBlur to reduce noise and improve matching
	GaussianBlur(grayImg, grayImg, Size(5, 5), 0);
	GaussianBlur(grayTempl, grayTempl, Size(5, 5), 0);

	Mat result;
	std::vector<Point> matchLocations;
	double minVal, maxVal;
	Point minLoc, maxLoc;

	// Multi-scale matching
	for (double scale = 0.5; scale <= 1.5; scale += 0.1)
	{
		Mat resizedTempl;
		resize(grayTempl, resizedTempl, Size(), scale, scale, INTER_LINEAR);

		if (resizedTempl.rows > grayImg.rows || resizedTempl.cols > grayImg.cols)
		{
			continue;
		}

		matchTemplate(grayImg, resizedTempl, result, TM_CCOEFF_NORMED);
		nonMaximumSuppression(result, matchLocations, matchThreshold);

		for (const auto& loc : matchLocations)
		{
			rectangle(img, loc, Point(loc.x + resizedTempl.cols, loc.y + resizedTempl.rows), Scalar(0, 255, 0), 2);
		}
	}

	if (!matchLocations.empty())
	{
		return true;
	}

	return false;
}

bool checkForSpecifiedColor(const Mat& img, int r_min, int r_max, int g_min, int g_max, int b_min, int b_max) {
    bool found = false;

#pragma omp parallel for collapse(2) shared(found)
    for (int y = 0; y < img.rows; ++y) {
        for (int x = 0; x < img.cols; ++x) {
            if (found) {
#pragma omp cancel for
            }

            auto pixelColor = img.at<Vec3b>(y, x);
            int b = pixelColor[0];
            int g = pixelColor[1];
            int r = pixelColor[2];

            if (r >= r_min && r <= r_max && g >= g_min && g <= g_max && b >= b_min && b <= b_max) {
                found = true;
#pragma omp cancel for
            }
        }
    }
#pragma omp cancellation point for
    return found;
}

bool check_pause() {
    if (paused) {
        this_thread::sleep_for(chrono::milliseconds(100));
        return true;
    }
    return false;
}

void buy_item(bool foundMatch) {
    if (foundMatch) {
        click(binsetX, binsetY);
        this_thread::sleep_for(chrono::milliseconds(20));
        click(consetX, consetY);
        this_thread::sleep_for(chrono::milliseconds(125));
        cout << "." << '\n';
    }
}

void debug(Mat img) {
    const string output_folder = R"(C:\SB\SavedImages\)";
    static int saved_image_counter = 0;

    const string output_path = output_folder + "capture_" + to_string(saved_image_counter++) + ".png";
    save_image(img, output_path);

    this_thread::sleep_for(chrono::milliseconds(100));
}

int main() {
    cout << "Welcome dismay" << '\n';
    this_thread::sleep_for(chrono::seconds(2));

    potato_img = imread(image_path_potato, IMREAD_COLOR);
    if (potato_img.empty()) {
        cerr << "Failed to load potato image from path: " << image_path_potato << '\n';
        return -1;
    }

    thread kb_hook_thread(keyboard_hook);
    kb_hook_thread.detach();

    while (true) {
        if (check_pause()) continue;

        {  // Create a new scope for Mat objects
            Mat img = hwnd2mat(region);
            if (img.empty()) {
                cerr << "Failed to capture image." << '\n';
                continue;
            }

            // Detect "potato" in the captured image
            bool potatoDetected = detectPotato(img, potato_img, 0.8);
            if (potatoDetected) {
                cout << "Potato detected!" << endl;
                keybd_event(VK_ESCAPE, 0, 0, 0);
                keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
                this_thread::sleep_for(chrono::milliseconds(150));
            }

            // Check for specific color
            bool colorPresent = checkForSpecifiedColor(img, r_min, r_max, g_min, g_max, b_min, b_max);
            if (colorPresent) {
                buy_item(true);
            }

            img.release();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
