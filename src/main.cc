#include <napi.h>

#include "keypress.h"
#include "microsleep.h"
#include "MMBitmap.h"
#include "mouse.h"
#include "screen.h"
#include "screengrab.h"
#include "window_manager.h"

int mouseDelay = 10;
int keyboardDelay = 10;

/*
 __  __
|  \/  | ___  _   _ ___  ___
| |\/| |/ _ \| | | / __|/ _ \
| |  | | (_) | |_| \__ \  __/
|_|  |_|\___/ \__,_|___/\___|

*/

int CheckMouseButton(const std::string &buttonString, MMMouseButton *const button)
{
	if (buttonString.empty())
		return -1;

	if (buttonString.compare("left") == 0)
	{
		*button = LEFT_BUTTON;
	}
	else if (buttonString.compare("right") == 0)
	{
		*button = RIGHT_BUTTON;
	}
	else if (buttonString.compare("middle") == 0)
	{
		*button = CENTER_BUTTON;
	}
	else
	{
		return -2;
	}

	return 0;
}

Napi::Number _dragMouse(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	if (info.Length() < 2 || info.Length() > 3)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	const size_t x = info[0].As<Napi::Number>().Int32Value();
	const size_t y = info[1].As<Napi::Number>().Int32Value();
	MMMouseButton button = LEFT_BUTTON;

	if (info.Length() == 3)
	{
		std::string buttonString = info[2].As<Napi::String>();

		switch (CheckMouseButton(buttonString, &button))
		{
		case -1:
			throw Napi::Error::New(env, "Null pointer in mouse button code.");
			break;
		case -2:
			throw Napi::Error::New(env, "Invalid mouse button specified.");
		}
	}

	MMPoint point = MMPointMake(x, y);
	dragMouse(point, button);
	microsleep(mouseDelay);

	return Napi::Number::New(env, 1);
}

Napi::Number _moveMouse(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	if (info.Length() != 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	size_t x = info[0].As<Napi::Number>().Int32Value();
	size_t y = info[1].As<Napi::Number>().Int32Value();

	MMPoint point;
	point = MMPointMake(x, y);
	moveMouse(point);
	microsleep(mouseDelay);

	return Napi::Number::New(env, 1);
}

Napi::Object _getMousePos(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	MMPoint pos = getMousePos();

	//Return object with .x and .y.
	Napi::Object obj = Napi::Object::New(env);

	obj.Set("x", (int)pos.x);
	obj.Set("y", (int)pos.y);

	return obj;
}

Napi::Number _mouseClick(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	MMMouseButton button = LEFT_BUTTON;
	bool doubleC = false;

	if (info.Length() > 0)
	{
		std::string buttonString = info[0].As<Napi::String>();

		switch (CheckMouseButton(buttonString, &button))
		{
		case -1:
			throw Napi::Error::New(env, "Null pointer in mouse button code.");
			break;
		case -2:
			throw Napi::Error::New(env, "Invalid mouse button specified.");
			break;
		}
	}

	if (info.Length() == 2)
	{
		doubleC = info[1].As<Napi::Boolean>().Value();
	}
	else if (info.Length() > 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	if (!doubleC)
	{
		clickMouse(button);
	}
	else
	{
		doubleClick(button);
	}

	microsleep(mouseDelay);

	return Napi::Number::New(env, 1);
}

Napi::Number _mouseToggle(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	MMMouseButton button = LEFT_BUTTON;
	bool down = false;

	if (info.Length() > 0)
	{
		std::string directionString = info[0].As<Napi::String>();

		if (directionString.compare("down") == 0)
		{
			down = true;
		}
		else if (directionString.compare("up") == 0)
		{
			down = false;
		}
		else
		{
			throw Napi::Error::New(env, "Invalid mouse button state specified.");
		}
	}

	if (info.Length() == 2)
	{
		std::string buttonString = info[1].As<Napi::String>();

		switch (CheckMouseButton(buttonString, &button))
		{
		case -1:
			throw Napi::Error::New(env, "Null pointer in mouse button code.");
			break;
		case -2:
			throw Napi::Error::New(env, "Invalid mouse button specified.");
			break;
		}
	}
	else if (info.Length() > 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	toggleMouse(down, button);
	microsleep(mouseDelay);

	return Napi::Number::New(env, 1);
}

Napi::Number _setMouseDelay(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	if (info.Length() != 1)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	mouseDelay = info[0].As<Napi::Number>().Int32Value();

	return Napi::Number::New(env, 1);
}

Napi::Number _scrollMouse(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	if (info.Length() != 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	int x = info[0].As<Napi::Number>().Int32Value();
	int y = info[1].As<Napi::Number>().Int32Value();

	scrollMouse(x, y);
	microsleep(mouseDelay);

	return Napi::Number::New(env, 1);
}

/*
 _  __          _                         _
| |/ /___ _   _| |__   ___   __ _ _ __ __| |
| ' // _ \ | | | '_ \ / _ \ / _` | '__/ _` |
| . \  __/ |_| | |_) | (_) | (_| | | | (_| |
|_|\_\___|\__, |_.__/ \___/ \__,_|_|  \__,_|
          |___/
*/
struct KeyNames
{
	const char *name;
	MMKeyCode key;
};

static KeyNames key_names[] =
	{
		{"backspace", K_BACKSPACE},
		{"delete", K_DELETE},
		{"enter", K_RETURN},
		{"tab", K_TAB},
		{"escape", K_ESCAPE},
		{"up", K_UP},
		{"down", K_DOWN},
		{"right", K_RIGHT},
		{"left", K_LEFT},
		{"home", K_HOME},
		{"end", K_END},
		{"pageup", K_PAGEUP},
		{"pagedown", K_PAGEDOWN},
		{"f1", K_F1},
		{"f2", K_F2},
		{"f3", K_F3},
		{"f4", K_F4},
		{"f5", K_F5},
		{"f6", K_F6},
		{"f7", K_F7},
		{"f8", K_F8},
		{"f9", K_F9},
		{"f10", K_F10},
		{"f11", K_F11},
		{"f12", K_F12},
		{"f13", K_F13},
		{"f14", K_F14},
		{"f15", K_F15},
		{"f16", K_F16},
		{"f17", K_F17},
		{"f18", K_F18},
		{"f19", K_F19},
		{"f20", K_F20},
		{"f21", K_F21},
		{"f22", K_F22},
		{"f23", K_F23},
		{"f24", K_F24},
		{"command", K_META},
		{"alt", K_ALT},
		{"control", K_CONTROL},
		{"shift", K_SHIFT},
		{"right_shift", K_RIGHTSHIFT},
		{"space", K_SPACE},
		{"printscreen", K_PRINTSCREEN},
		{"insert", K_INSERT},
		{"menu", K_MENU},

		{"audio_mute", K_AUDIO_VOLUME_MUTE},
		{"audio_vol_down", K_AUDIO_VOLUME_DOWN},
		{"audio_vol_up", K_AUDIO_VOLUME_UP},
		{"audio_play", K_AUDIO_PLAY},
		{"audio_stop", K_AUDIO_STOP},
		{"audio_pause", K_AUDIO_PAUSE},
		{"audio_prev", K_AUDIO_PREV},
		{"audio_next", K_AUDIO_NEXT},
		{"audio_rewind", K_AUDIO_REWIND},
		{"audio_forward", K_AUDIO_FORWARD},
		{"audio_repeat", K_AUDIO_REPEAT},
		{"audio_random", K_AUDIO_RANDOM},

		{"numpad_0", K_NUMPAD_0},
		{"numpad_1", K_NUMPAD_1},
		{"numpad_2", K_NUMPAD_2},
		{"numpad_3", K_NUMPAD_3},
		{"numpad_4", K_NUMPAD_4},
		{"numpad_5", K_NUMPAD_5},
		{"numpad_6", K_NUMPAD_6},
		{"numpad_7", K_NUMPAD_7},
		{"numpad_8", K_NUMPAD_8},
		{"numpad_9", K_NUMPAD_9},

		{"lights_mon_up", K_LIGHTS_MON_UP},
		{"lights_mon_down", K_LIGHTS_MON_DOWN},
		{"lights_kbd_toggle", K_LIGHTS_KBD_TOGGLE},
		{"lights_kbd_up", K_LIGHTS_KBD_UP},
		{"lights_kbd_down", K_LIGHTS_KBD_DOWN},

		{NULL, K_NOT_A_KEY} /* end marker */
};

int CheckKeyCodes(std::string &keyName, MMKeyCode *key)
{
	if (!key)
		return -1;

	if (keyName.length() == 1)
	{
		*key = keyCodeForChar(*keyName.c_str());
		return 0;
	}

	*key = K_NOT_A_KEY;

	KeyNames *kn = key_names;
	while (kn->name)
	{
		if (keyName.compare(kn->name) == 0)
		{
			*key = kn->key;
			break;
		}
		kn++;
	}

	if (*key == K_NOT_A_KEY)
	{
		return -2;
	}

	return 0;
}

int CheckKeyFlags(std::string &flagString, MMKeyFlags *flags)
{
	if (!flags)
		return -1;

	if (flagString.compare("alt") == 0)
	{
		*flags = MOD_ALT;
	}
	else if (flagString.compare("command") == 0)
	{
		*flags = MOD_META;
	}
	else if (flagString.compare("control") == 0)
	{
		*flags = MOD_CONTROL;
	}
	else if (flagString.compare("shift") == 0)
	{
		*flags = MOD_SHIFT;
	}
	else if (flagString.compare("none") == 0)
	{
		*flags = MOD_NONE;
	}
	else
	{
		return -2;
	}

	return 0;
}

int GetFlagsFromString(Napi::Value value, MMKeyFlags *flags)
{
	std::string fstr = value.As<Napi::String>();
	return CheckKeyFlags(fstr, flags);
}

int GetFlagsFromValue(Napi::Value value, MMKeyFlags *flags)
{
	if (!flags)
		return -1;

	//Optionally allow an array of flag strings to be passed.
	if (value.IsArray())
	{
		Napi::Array a = value.As<Napi::Array>();
		for (uint32_t i = 0; i < a.Length(); ++i)
		{
			Napi::Value v = a.Get(i);
			if (!v.IsString())
				return -2;

			MMKeyFlags f = MOD_NONE;
			const int rv = GetFlagsFromString(v, &f);
			if (rv)
				return rv;

			*flags = (MMKeyFlags)(*flags | f);
		}
		return 0;
	}

	//If it's not an array, it should be a single string value.
	return GetFlagsFromString(value, flags);
}

Napi::Number _keyTap(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	MMKeyFlags flags = MOD_NONE;
	MMKeyCode key;

	std::string keyName = info[0].As<Napi::String>();

	switch (info.Length())
	{
	case 2:
		switch (GetFlagsFromValue(info[1], &flags))
		{
		case -1:
			throw Napi::Error::New(env, "Null pointer in key flag.");
			break;
		case -2:
			throw Napi::Error::New(env, "Invalid key flag specified.");
			break;
		}
		break;
	case 1:
		break;
	default:
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	switch (CheckKeyCodes(keyName, &key))
	{
	case -1:
		throw Napi::Error::New(env, "Null pointer in key code.");
		break;
	case -2:
		throw Napi::Error::New(env, "Invalid key code specified.");
		break;
	default:
		tapKeyCode(key, flags);
	}

	return Napi::Number::New(env, 1);
}

Napi::Number _keyToggle(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	MMKeyFlags flags = MOD_NONE;
	MMKeyCode key;

	bool down = false;

	//Get arguments from JavaScript.
	std::string keyName = info[0].As<Napi::String>();

	//Check and confirm number of arguments.
	switch (info.Length())
	{
	case 3:
		//Get key modifier.
		switch (GetFlagsFromValue(info[2], &flags))
		{
		case -1:
			throw Napi::Error::New(env, "Null pointer in key flag.");
			break;
		case -2:
			throw Napi::Error::New(env, "Invalid key flag specified.");
			break;
		}
		break;
	case 2:
		break;
	default:
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	//Get down value if provided.
	if (info.Length() > 1)
	{
		std::string directionString = info[1].As<Napi::String>();

		if (directionString.compare("down") == 0)
		{
			down = true;
		}
		else if (directionString.compare("up") == 0)
		{
			down = false;
		}
		else
		{
			throw Napi::Error::New(env, "Invalid key state specified.");
		}
	}

	//Get the acutal key.
	switch (CheckKeyCodes(keyName, &key))
	{
	case -1:
		throw Napi::Error::New(env, "Null pointer in key code.");
		break;
	case -2:
		throw Napi::Error::New(env, "Invalid key code specified.");
		break;
	default:
		toggleKeyCode(key, down, flags);
	}

	return Napi::Number::New(env, 1);
}

Napi::Number _typeString(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	std::string stringToType = info[0].As<Napi::String>();

	typeString(stringToType.c_str());

	return Napi::Number::New(env, 1);
}

Napi::Number _typeStringDelayed(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	std::string stringToType = info[0].As<Napi::String>();
	size_t cpm = info[1].As<Napi::Number>().Int32Value();

	typeStringDelayed(stringToType.c_str(), (unsigned int)cpm);

	return Napi::Number::New(env, 1);
}

Napi::Number _setKeyboardDelay(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	if (info.Length() != 1)
	{
		throw Napi::Error::New(env, "Invalid number of arguments.");
	}

	keyboardDelay = info[0].As<Napi::Number>().Int32Value();

	return Napi::Number::New(env, 1);
}

/*
  ____
 / ___|  ___ _ __ ___  ___ _ __
 \___ \ / __| '__/ _ \/ _ \ '_ \
  ___) | (__| | |  __/  __/ | | |
 |____/ \___|_|  \___|\___|_| |_|

*/

Napi::Object _getScreenSize(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	//Get display size.
	MMSize displaySize = getMainDisplaySize();

	//Create our return object.
	Napi::Object obj = Napi::Object::New(env);
	obj.Set(Napi::String::New(env, "width"), Napi::Number::New(env, (double)displaySize.width));
	obj.Set(Napi::String::New(env, "height"), Napi::Number::New(env, (double)displaySize.height));

	return obj;
}

Napi::String _getXDisplayName(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

#if defined(USE_X11)
	std::string display = getXDisplay();
	return Napi::String::New(env, display);
#else
	throw Napi::Error::New(env, "getXDisplayName is only supported on Linux");
#endif
}

Napi::Number _setXDisplayName(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

#if defined(USE_X11)
	std::string displayName = info[0].As<Napi::String>();
	setXDisplay(displayName.c_str());
	return Napi::Number::New(env, 1);
#else
	throw Napi::Error::New(env, "setXDisplayName is only supported on Linux");
#endif
}

Napi::Number _highlight(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	long duration;
	float opacity;

	if (info.Length() == 6)
	{
		x = info[0].As<Napi::Number>().Int32Value();
		y = info[1].As<Napi::Number>().Int32Value();
		width = info[2].As<Napi::Number>().Int32Value();
		height = info[3].As<Napi::Number>().Int32Value();
		duration = (int)info[4].As<Napi::Number>().Int64Value();
		opacity = info[5].As<Napi::Number>().FloatValue();

		highlight(x, y, width, height, duration, opacity);
		return Napi::Number::New(env, 1);
	}
	return Napi::Number::New(env, 1);
}

Napi::Number _getActiveWindow(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();

	WindowHandle windowHandle = getActiveWindow();
	return Napi::Number::New(env, (double)windowHandle);
}

Napi::Array _getWindows(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();

	std::vector<WindowHandle> windowHandles = getWindows();
	auto arr = Napi::Array::New(env, windowHandles.size());

	for (size_t idx = 0; idx < windowHandles.size(); ++idx) {
		arr[(uint32_t)idx] = windowHandles[idx];
	}

	return arr;
}

Napi::Object _getWindowRect(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();
	
	WindowHandle windowHandle = info[0].As<Napi::Number>().Int64Value();
	MMRect windowRect = getWindowRect(windowHandle);

	Napi::Object obj = Napi::Object::New(env);
	obj.Set(Napi::String::New(env, "x"), Napi::Number::New(env, (double)windowRect.origin.x));
	obj.Set(Napi::String::New(env, "y"), Napi::Number::New(env, (double)windowRect.origin.y));
	obj.Set(Napi::String::New(env, "width"), Napi::Number::New(env, (double)windowRect.size.width));
	obj.Set(Napi::String::New(env, "height"), Napi::Number::New(env, (double)windowRect.size.height));

	return obj;
}

Napi::String _getWindowTitle(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();

	WindowHandle windowHandle = info[0].As<Napi::Number>().Int64Value();
	return Napi::String::New(env, getWindowTitle(windowHandle));
}

Napi::Object _captureScreen(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	int64_t x;
	int64_t y;
	int64_t w;
	int64_t h;

	MMSize displaySize = getMainDisplaySize();
	//If user has provided screen coords, use them!
	if (info.Length() == 4)
	{
		x = info[0].As<Napi::Number>().Int64Value();
		y = info[1].As<Napi::Number>().Int64Value();
		w = info[2].As<Napi::Number>().Int64Value();
		h = info[3].As<Napi::Number>().Int64Value();

		if (!(x >= 0 && x <= displaySize.width))
		{
			throw Napi::Error::New(env, "Error: x coordinate outside of display");
		}
		if (!(y >= 0 && y <= displaySize.height))
		{
			throw Napi::Error::New(env, "Error: y coordinate outside of display");
		}
		if (!((x + w) >= 0 && (x + w) <= displaySize.width))
		{
			throw Napi::Error::New(env, "Error: Given width exceeds display dimensions");
		}
		if (!((y + h) >= 0 && (y + h) <= displaySize.height))
		{
			throw Napi::Error::New(env, "Error: Given height exceeds display dimensions");
		}
	}
	else
	{
		//We're getting the full screen.
		x = 0;
		y = 0;

		//Get screen size.
		w = displaySize.width;
		h = displaySize.height;
	}

	MMBitmapRef bitmap = copyMMBitmapFromDisplayInRect(MMRectMake(x, y, w, h));
	
	if (bitmap == NULL) {
		throw Napi::Error::New(env, "Error: Failed to capture screen");
	}

	uint64_t bufferSize = bitmap->bytewidth * bitmap->height;
	Napi::Buffer<char> buffer = Napi::Buffer<char>::Copy(env, (char *)bitmap->imageBuffer, bufferSize);

	Napi::Object obj = Napi::Object::New(env);
	obj.Set(Napi::String::New(env, "width"), Napi::Number::New(env, (double)bitmap->width));
	obj.Set(Napi::String::New(env, "height"), Napi::Number::New(env, (double)bitmap->height));
	obj.Set(Napi::String::New(env, "byteWidth"), Napi::Number::New(env, (double)bitmap->bytewidth));
	obj.Set(Napi::String::New(env, "bitsPerPixel"), Napi::Number::New(env, bitmap->bitsPerPixel));
	obj.Set(Napi::String::New(env, "bytesPerPixel"), Napi::Number::New(env, bitmap->bytesPerPixel));
	obj.Set(Napi::String::New(env, "image"), buffer);

	destroyMMBitmap(bitmap);

	return obj;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "dragMouse"), Napi::Function::New(env, _dragMouse));
	exports.Set(Napi::String::New(env, "moveMouse"), Napi::Function::New(env, _moveMouse));
	exports.Set(Napi::String::New(env, "getMousePos"), Napi::Function::New(env, _getMousePos));
	exports.Set(Napi::String::New(env, "mouseClick"), Napi::Function::New(env, _mouseClick));
	exports.Set(Napi::String::New(env, "mouseToggle"), Napi::Function::New(env, _mouseToggle));
	exports.Set(Napi::String::New(env, "scrollMouse"), Napi::Function::New(env, _scrollMouse));
	exports.Set(Napi::String::New(env, "setMouseDelay"), Napi::Function::New(env, _setMouseDelay));

	exports.Set(Napi::String::New(env, "keyTap"), Napi::Function::New(env, _keyTap));
	exports.Set(Napi::String::New(env, "keyToggle"), Napi::Function::New(env, _keyToggle));
	exports.Set(Napi::String::New(env, "typeString"), Napi::Function::New(env, _typeString));
	exports.Set(Napi::String::New(env, "typeStringDelayed"), Napi::Function::New(env, _typeStringDelayed));
	exports.Set(Napi::String::New(env, "setKeyboardDelay"), Napi::Function::New(env, _setKeyboardDelay));

	exports.Set(Napi::String::New(env, "getScreenSize"), Napi::Function::New(env, _getScreenSize));
	exports.Set(Napi::String::New(env, "highlight"), Napi::Function::New(env, _highlight));
	exports.Set(Napi::String::New(env, "getWindows"), Napi::Function::New(env, _getWindows));
	exports.Set(Napi::String::New(env, "getActiveWindow"), Napi::Function::New(env, _getActiveWindow));
	exports.Set(Napi::String::New(env, "getWindowRect"), Napi::Function::New(env, _getWindowRect));
	exports.Set(Napi::String::New(env, "getWindowTitle"), Napi::Function::New(env, _getWindowTitle));
	exports.Set(Napi::String::New(env, "captureScreen"), Napi::Function::New(env, _captureScreen));
	exports.Set(Napi::String::New(env, "getXDisplayName"), Napi::Function::New(env, _getXDisplayName));
	exports.Set(Napi::String::New(env, "setXDisplayName"), Napi::Function::New(env, _setXDisplayName));

	return exports;
}

NODE_API_MODULE(libnut, Init)
