#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "../window_manager.h"
extern "C" {
	#include "../xdisplay.h"
}

WindowHandle getActiveWindow() {
	Display* display;
	Window focus;
	int32_t revert_to;

	display = XGetMainDisplay();
	if (display == NULL) {
		return -1;
	}

	XGetInputFocus(display, &focus, &revert_to);
	return focus;
}

std::vector<WindowHandle> getWindowsRecursive(Display* display, Window w) {
	std::vector<WindowHandle> windowHandles;
	std::vector<WindowHandle> childrenWindowHandles;
	Window root, parent;
	Window* children;
	uint32_t nchildren;

	if (!XQueryTree(display, w, &root, &parent, &children, &nchildren)) {
		return windowHandles;
	}

	if (children != NULL) {
		for (uint32_t i = 0; i < nchildren; i++) {
			windowHandles.push_back(children[i]);
			childrenWindowHandles = getWindowsRecursive(display, children[i]);
			windowHandles.insert(windowHandles.end(), childrenWindowHandles.begin(), childrenWindowHandles.end());
		}
		XFree(children);
	}

	return windowHandles;
}

std::vector<WindowHandle> getWindows() {
	Display* display;
	Window root;
	std::vector<WindowHandle> windowHandles;

	display = XGetMainDisplay();
	if (display == NULL) {
		return windowHandles;
	}

	root = DefaultRootWindow(display);
	windowHandles = getWindowsRecursive(display, root);

	return windowHandles;
}

std::string getWindowTitle(const WindowHandle windowHandle) {
	Display* display;
	XTextProperty text_prop;

	display = XGetMainDisplay();
	if (display == NULL || windowHandle < 0) {
		return "";
	}

	/*
	* While there's also `XFetchName` to retrieve a window's `WM_NAME` property, in my tests `XFetchName` always failed and return 0
	* `XGetWMName` on the other hand just worked as expected.
	* Keep that in mind in case you're considering changing this implementation
	*/
	if (!XGetWMName(display, windowHandle, &text_prop)) {
		return "";
	}

	return std::string(reinterpret_cast<const char*>(text_prop.value));
}

MMPoint getAbsoluteOriginRecursive(Display* display, const WindowHandle w, MMPoint origin) {
	Window root, parent;
	Window* children;
	int32_t x, y;
	uint32_t width, height, border_width, border_height, nchildren;

	if (!XQueryTree(display, w, &root, &parent, &children, &nchildren)) {
		return MMPointMake(0,0);
	}
	if (children != NULL) {
		XFree(children);
	}
	if (parent == root) {
		return origin;
	}

	if (!XGetGeometry(display, parent, &root, &x, &y, &width, &height, &border_width, &border_height)) {
		return MMPointMake(0,0);
	}
	origin.x += x;
	origin.y += y;

	return getAbsoluteOriginRecursive(display, parent, origin);
}

MMRect getWindowRect(const WindowHandle w) {
	Display* display;
	Window root;
	int32_t x, y;
	uint32_t width, height, border_width, border_height;
	MMPoint absoluteOrigin;

	display = XGetMainDisplay();
	if (display == NULL || w < 0) {
		return MMRectMake(0, 0, 0, 0);
	}

	if (!XGetGeometry(display, w, &root, &x, &y, &width, &height, &border_width, &border_height)) {
		return MMRectMake(0, 0, 0, 0);
	}
	absoluteOrigin = getAbsoluteOriginRecursive(display, w, MMPointMake(x, y));

	return MMRectMake(absoluteOrigin.x, absoluteOrigin.y, width, height);
}
