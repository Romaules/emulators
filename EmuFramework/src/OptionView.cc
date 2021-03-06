/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
#include <imagine/gui/TextEntry.hh>
#include <algorithm>
#ifdef __ANDROID__
#include <imagine/base/android/RootCpufreqParamSetter.hh>
#endif

using namespace IG;

static FS::PathString savePathStrToDescStr(char *savePathStr)
{
	FS::PathString desc{};
	if(strlen(savePathStr))
	{
		if(string_equal(savePathStr, optionSavePathDefaultToken))
			string_copy(desc, "Default");
		else
		{
			string_copy(desc, FS::basename(optionSavePath).data());
		}
	}
	else
	{
		string_copy(desc, "Same as Game");
	}
	return desc;
}

void BiosSelectMenu::onSelectFile(const char* name, Input::Event e)
{
	logMsg("size %d", (int)sizeof(*biosPathStr));
	string_printf(*biosPathStr, "%s/%s", FS::current_path().data(), name);
	if(onBiosChangeD) onBiosChangeD();
	workDirStack.pop();
	viewStack.popAndShow();
}

void BiosSelectMenu::init(FS::PathString *biosPathStr, EmuNameFilterFunc fsFilter)
{
	var_selfs(biosPathStr);
	var_selfs(fsFilter);
	init();
}

void BiosSelectMenu::init()
{
	assert(biosPathStr);
	choiceEntry[0].init("Select File"); choiceEntryItem[0] = &choiceEntry[0];
	choiceEntry[0].onSelect() =
		[this](TextMenuItem &, View &, Input::Event e)
		{
			workDirStack.push();
			chdirFromFilePath(biosPathStr->data());
			auto &fPicker = *new EmuFilePicker{window()};
			fPicker.init(false, fsFilter);
			fPicker.setOnSelectFile(
				[this](FSPicker &picker, const char* name, Input::Event e)
				{
					onSelectFile(name, e);
					picker.dismiss();
				});
			fPicker.setOnClose(
				[](FSPicker &picker, Input::Event e)
				{
					picker.dismiss();
					workDirStack.pop();
				});
			modalViewController.pushAndShow(fPicker, e);
		};
	choiceEntry[1].init("Unset"); choiceEntryItem[1] = &choiceEntry[1];
	choiceEntry[1].onSelect() =
		[this](TextMenuItem &, View &, Input::Event e)
		{
			strcpy(biosPathStr->data(), "");
			auto onBiosChange = onBiosChangeD;
			popAndShow();
			onBiosChange.callSafe();
		};
	TableView::init(choiceEntryItem, sizeofArray(choiceEntry));
}

void OptionView::autoSaveStateInit()
{
	static const char *str[] =
	{
		"Off", "Game Exit",
		"15mins", "30mins"
	};
	int val = 0;
	switch(optionAutoSaveState.val)
	{
		bcase 1: val = 1;
		bcase 15: val = 2;
		bcase 30: val = 3;
	}
	autoSaveState.init(str, val, sizeofArray(str));
}

void OptionView::fastForwardSpeedinit()
{
	static const char *str[] =
	{
		"3x", "4x", "5x",
		"6x", "7x", "8x"
	};
	int val = 0;
	if(optionFastForwardSpeed >= MIN_FAST_FORWARD_SPEED && optionFastForwardSpeed <= 7)
	{
		val = optionFastForwardSpeed - MIN_FAST_FORWARD_SPEED;
	}
	fastForwardSpeed.init(str, val, sizeofArray(str));
}


static void uiVisibiltyInit(const Byte1Option &option, MultiChoiceSelectMenuItem &menuItem)
{
	static const char *str[] =
	{
		"Off", "In Game", "On",
	};
	int val = 2;
	if(option < 2)
		val = option;
	menuItem.init(str, val, sizeofArray(str));
}

#ifdef __ANDROID__
void OptionView::androidTextureStorageInit()
{
	static const char *str[]
	{
		"Auto",
		"Standard",
		"Graphic Buffer",
		"Surface Texture"
	};
	int val = optionAndroidTextureStorage;
	androidTextureStorage.init(str, val, Base::androidSDK() >= 14 ? sizeofArray(str) : sizeofArray(str)-1);
}
#endif

#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
void OptionView::frameIntervalInit()
{
	static const char *str[] =
	{
		"Full", "1/2", "1/3", "1/4"
	};
	int baseVal = 1;
	int val = int(optionFrameInterval);
	frameInterval.init(str, val, sizeofArray(str), baseVal);
}
#endif

void OptionView::audioRateInit()
{
	static const char *str[] =
	{
		"22KHz", "32KHz", "44KHz", "48KHz"
	};
	int rates = 3;
	if(AudioManager::nativeFormat().rate >= 48000)
	{
		logMsg("supports 48KHz");
		rates++;
	}

	int val = 2; // default to 44KHz
	switch(optionSoundRate)
	{
		bcase 22050: val = 0;
		bcase 32000: val = 1;
		bcase 48000: val = 3;
	}

	audioRate.init(str, val, rates);
}

enum { O_AUTO = -1, O_90, O_270, O_0, O_180 };

int convertOrientationMenuValueToOption(int val)
{
	if(val == O_AUTO)
		return Base::VIEW_ROTATE_AUTO;
	else if(val == O_90)
		return Base::VIEW_ROTATE_90;
	else if(val == O_270)
		return Base::VIEW_ROTATE_270;
	else if(val == O_0)
		return Base::VIEW_ROTATE_0;
	else if(val == O_180)
		return Base::VIEW_ROTATE_180;
	bug_exit("invalid value %d", val);
	return 0;
}

static void orientationInit(MultiChoiceSelectMenuItem &item, uint option)
{
	static const char *str[] =
	{
		#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
		"Auto",
		#endif

		#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || (defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2)
		"Landscape", "Landscape 2", "Portrait", "Portrait 2"
		#else
		"90 Left", "90 Right", "Standard", "Upside Down"
		#endif
	};
	int baseVal = 0;
	#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
	baseVal = -1;
	#endif
	uint initVal = O_AUTO;
	if(option == Base::VIEW_ROTATE_90)
		initVal = O_90;
	else if(option == Base::VIEW_ROTATE_270)
		initVal = O_270;
	else if(option == Base::VIEW_ROTATE_0)
		initVal = O_0;
	else if(option == Base::VIEW_ROTATE_180)
		initVal = O_180;
	item.init(str, initVal, sizeofArray(str), baseVal);
}

void OptionView::gameOrientationInit()
{
	orientationInit(gameOrientation, optionGameOrientation);
}

void OptionView::menuOrientationInit()
{
	orientationInit(menuOrientation, optionMenuOrientation);
}

void OptionView::aspectRatioInit()
{
	assert(sizeofArray(aspectRatioStr) >= EmuSystem::aspectRatioInfos);
	int val = 0;
	iterateTimes(EmuSystem::aspectRatioInfos, i)
	{
		aspectRatioStr[i] = EmuSystem::aspectRatioInfo[i].name;
		if(optionAspectRatio == EmuSystem::aspectRatioInfo[i].aspect)
		{
			val = i;
		}
	}
	aspectRatio.init(aspectRatioStr, val, EmuSystem::aspectRatioInfos);
}

#ifdef CONFIG_AUDIO_LATENCY_HINT
void OptionView::soundBuffersInit()
{
	static const char *str2[] = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
	static const char *str3[] = { "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
	static_assert(sizeofArray(str2) == 12 - (OPTION_SOUND_BUFFERS_MIN-1) || sizeofArray(str3) == 12 - (OPTION_SOUND_BUFFERS_MIN-1), "incorrect sound buffers string array");
	soundBuffers.init((OPTION_SOUND_BUFFERS_MIN == 2) ? str2 : str3, std::max((int)optionSoundBuffers - (int)OPTION_SOUND_BUFFERS_MIN, 0), (OPTION_SOUND_BUFFERS_MIN == 2) ? sizeofArray(str2) : sizeofArray(str3));
}
#endif

void OptionView::zoomInit()
{
	static const char *str[] = { "100%", "90%", "80%", "70%", "Integer-only", "Integer-only (Height)" };
	int val = 0;
	switch(optionImageZoom.val)
	{
		bcase 100: val = 0;
		bcase 90: val = 1;
		bcase 80: val = 2;
		bcase 70: val = 3;
		bcase optionImageZoomIntegerOnly: val = 4;
		bcase optionImageZoomIntegerOnlyY: val = 5;
	}
	zoom.init(str, val, sizeofArray(str));
}

void OptionView::viewportZoomInit()
{
	static const char *str[] = { "100%", "95%", "90%", "85%" };
	int val = 0;
	switch(optionViewportZoom.val)
	{
		bcase 100: val = 0;
		bcase 95: val = 1;
		bcase 90: val = 2;
		bcase 85: val = 3;
	}
	viewportZoom.init(str, val, sizeofArray(str));
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void OptionView::imgEffectInit()
{
	static const char *str[] {"Off", "hq2x", "Scale2x", "Prescale 2x"};
	uint init = 0;
	switch(optionImgEffect)
	{
		bcase VideoImageEffect::HQ2X: init = 1;
		bcase VideoImageEffect::SCALE2X: init = 2;
		bcase VideoImageEffect::PRESCALE2X: init = 3;
	}
	imgEffect.init(str, init, sizeofArray(str));
}
#endif

void OptionView::overlayEffectInit()
{
	static const char *str[] = { "Off", "Scanlines", "Scanlines 2x", "CRT Mask", "CRT", "CRT 2x" };
	uint init = 0;
	switch(optionOverlayEffect)
	{
		bcase VideoImageOverlay::SCANLINES: init = 1;
		bcase VideoImageOverlay::SCANLINES_2: init = 2;
		bcase VideoImageOverlay::CRT: init = 3;
		bcase VideoImageOverlay::CRT_RGB: init = 4;
		bcase VideoImageOverlay::CRT_RGB_2: init = 5;
	}
	overlayEffect.init(str, init, sizeofArray(str));
}

void OptionView::overlayEffectLevelInit()
{
	static const char *str[] = { "10%", "25%", "33%", "50%", "66%", "75%", "100%" };
	uint init = 0;
	switch(optionOverlayEffectLevel)
	{
		bcase 25: init = 1;
		bcase 33: init = 2;
		bcase 50: init = 3;
		bcase 66: init = 4;
		bcase 75: init = 5;
		bcase 100: init = 6;
	}
	overlayEffectLevel.init(str, init, sizeofArray(str));
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void OptionView::imgEffectPixelFormatInit()
{
	static const char *str[]
	{
		"Auto", "RGB 16", "RGB 24"
	};
	uint init = 0;
	switch(optionImageEffectPixelFormat.val)
	{
		bcase PIXEL_RGB565: init = 1;
		bcase PIXEL_RGBA8888: init = 2;
	}
	imgEffectPixelFormat.init(str, init, sizeofArray(str));
}
#endif

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
void OptionView::windowPixelFormatInit()
{
	static const char *str[]
	{
		"Auto", "RGB 16", "RGB 24"
	};
	uint init = 0;
	switch(optionWindowPixelFormat.val)
	{
		bcase PIXEL_RGB565: init = 1;
		bcase PIXEL_RGB888: init = 2;
	}
	windowPixelFormat.init(str, init, sizeofArray(str));
}
#endif

void OptionView::fontSizeInit()
{
	static const char *str[] =
	{
			"2", "2.5",
			"3", "3.5",
			"4", "4.5",
			"5", "5.5",
			"6", "6.5",
			"7", "7.5",
			"8", "8.5",
			"9", "9.5",
			"10", "10.50",
	};
	uint init = 0;
	switch(optionFontSize)
	{
		bcase 2500: init = 1;
		bcase 3000: init = 2;
		bcase 3500: init = 3;
		bcase 4000: init = 4;
		bcase 4500: init = 5;
		bcase 5000: init = 6;
		bcase 5500: init = 7;
		bcase 6000: init = 8;
		bcase 6500: init = 9;
		bcase 7000: init = 10;
		bcase 7500: init = 11;
		bcase 8000: init = 12;
		bcase 8500: init = 13;
		bcase 9000: init = 14;
		bcase 9500: init = 15;
		bcase 10000: init = 16;
		bcase 10500: init = 17;
	}
	fontSize.init(str, init, sizeofArray(str));
}

#if defined CONFIG_BASE_ANDROID
void OptionView::processPriorityInit()
{
	static const char *str[] = { "Normal", "High", "Very High" };
	auto prio = Base::processPriority();
	int init = 0;
	if(optionProcessPriority.val == 0)
		init = 0;
	if(optionProcessPriority.val == -6)
		init = 1;
	if(optionProcessPriority.val == -14)
		init = 2;
	processPriority.init(str, init, sizeofArray(str));
}
#endif

template <size_t S>
static void printPathMenuEntryStr(char (&str)[S])
{
	string_printf(str, "Save Path: %s", savePathStrToDescStr(optionSavePath).data());
}

static class SavePathSelectMenu
{
public:
	PathChangeDelegate onPathChange;

	constexpr SavePathSelectMenu() {}

	void onClose(Input::Event e)
	{
		snprintf(optionSavePath, sizeof(FS::PathString), "%s", FS::current_path().data());
		logMsg("set save path %s", (char*)optionSavePath);
		if(onPathChange) onPathChange(optionSavePath);
		workDirStack.pop();
	}

	void init(Input::Event e)
	{
		auto &multiChoiceView = *new MultiChoiceView{"Save Path", mainWin.win};
		multiChoiceView.init(3);
		multiChoiceView.setItem(0, "Set Custom Path",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				workDirStack.push();
				FS::current_path(optionSavePath);
				auto &fPicker = *new EmuFilePicker{mainWin.win};
				fPicker.init(true, {});
				fPicker.setOnClose(
					[this](FSPicker &picker, Input::Event e)
					{
						onClose(e);
						picker.dismiss();
						viewStack.popAndShow();
					});
				modalViewController.pushAndShow(fPicker, e);
			});
		multiChoiceView.setItem(1, "Same as Game",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto onPathChange = this->onPathChange;
				viewStack.popAndShow();
				strcpy(optionSavePath, "");
				if(onPathChange) onPathChange("");
			});
		multiChoiceView.setItem(2, "Default",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto onPathChange = this->onPathChange;
				viewStack.popAndShow();
				strcpy(optionSavePath, optionSavePathDefaultToken);
				if(onPathChange) onPathChange(optionSavePathDefaultToken);
			});
		viewStack.pushAndShow(multiChoiceView, e);
	}
} pathSelectMenu;

class DetectFrameRateView : public View
{
	IG::WindowRect viewFrame;

public:
	using DetectFrameRateDelegate = DelegateFunc<void (double frameRate)>;
	DetectFrameRateDelegate onDetectFrameTime;
	Base::Screen::OnFrameDelegate detectFrameRate;
	Base::FrameTimeBase totalFrameTime{};
	Gfx::Text fpsText;
	double lastAverageFrameTimeSecs = 0;
	uint totalFrames = 0;
	uint callbacks = 0;
	std::array<char, 32> fpsStr{};

	DetectFrameRateView(Base::Window &win): View(win) {}
	IG::WindowRect &viewRect() override { return viewFrame; }

	double totalFrameTimeSecs() const
	{
		return Base::frameTimeBaseToSecsDec(totalFrameTime);
	}

	double averageFrameTimeSecs() const
	{
		return totalFrameTimeSecs() / (double)totalFrames;
	}

	void init()
	{
		View::defaultFace->precacheAlphaNum();
		View::defaultFace->precache(".");
		fpsText.init(View::defaultFace);
		fpsText.setString("Preparing to detect frame rate...");
		detectFrameRate =
			[this](Base::Screen::FrameParams params)
			{
				const uint callbacksToSkip = 30;
				callbacks++;
				if(callbacks >= callbacksToSkip)
				{
					detectFrameRate =
						[this](Base::Screen::FrameParams params)
						{
							const uint framesToTime = 120 * 10;
							totalFrameTime += params.timestampDiff();
							totalFrames++;
							if(totalFrames % 120 == 0)
							{
								if(!lastAverageFrameTimeSecs)
									lastAverageFrameTimeSecs = averageFrameTimeSecs();
								else
								{
									double avgFrameTimeSecs = averageFrameTimeSecs();
									double avgFrameTimeDiff = std::abs(lastAverageFrameTimeSecs - avgFrameTimeSecs);
									if(avgFrameTimeDiff < 0.00001)
									{
										logMsg("finished with diff %.8f, total frame time: %.2f, average %.6f over %u frames",
											avgFrameTimeDiff, totalFrameTimeSecs(), avgFrameTimeSecs, totalFrames);
										onDetectFrameTime(avgFrameTimeSecs);
										popAndShow();
										return;
									}
									else
										lastAverageFrameTimeSecs = averageFrameTimeSecs();
								}
								if(totalFrames % 60 == 0)
								{
									string_printf(fpsStr, "%.2ffps", 1. / averageFrameTimeSecs());
									fpsText.setString(fpsStr.data());
									fpsText.compile(projP);
									postDraw();
								}
							}
							if(totalFrames >= framesToTime)
							{
								logErr("unstable frame rate over frame time: %.2f, average %.6f over %u frames",
									totalFrameTimeSecs(), averageFrameTimeSecs(), totalFrames);
								onDetectFrameTime(0);
								popAndShow();
							}
							else
							{
								params.readdOnFrame();
							}
						};
					params.screen().addOnFrame(detectFrameRate);
				}
				else
				{
					params.readdOnFrame();
				}
			};
		emuWin->win.screen()->addOnFrame(detectFrameRate);
	}

	void deinit() override
	{
		emuWin->win.screen()->removeOnFrame(detectFrameRate);
	}

	void place() override
	{
		fpsText.compile(projP);
	}

	void inputEvent(Input::Event e) override
	{
		if(e.pushed() && e.isDefaultCancelButton())
		{
			logMsg("aborted detection");
			popAndShow();
		}
	}

	void draw() override
	{
		using namespace Gfx;
		setColor(1., 1., 1., 1.);
		texAlphaProgram.use(projP.makeTranslate());
		fpsText.draw(projP.alignXToPixel(projP.bounds().xCenter()),
			projP.alignYToPixel(projP.bounds().yCenter()), C2DO, projP);
	}

	void onAddedToController(Input::Event e) override {}
};

static class FrameRateSelectMenu
{
public:
	using FrameRateChangeDelegate = DelegateFunc<void (double frameRate)>;
	FrameRateChangeDelegate onFrameTimeChange;

	constexpr FrameRateSelectMenu() {}

	void init(EmuSystem::VideoSystem vidSys, Input::Event e)
	{
		auto &multiChoiceView = *new MultiChoiceView{"Frame Rate", mainWin.win};
		const bool includeFrameRateDetection = !Config::envIsIOS;
		multiChoiceView.init(includeFrameRateDetection ? 4 : 3);
		multiChoiceView.setItem(0, "Set with screen's reported rate",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				if(!emuWin->win.screen()->frameRateIsReliable())
				{
					#ifdef __ANDROID__
					if(Base::androidSDK() <= 10)
					{
						popup.postError("Many Android 2.3 devices mis-report their refresh rate, "
							"using the detected or default rate may give better results");
					}
					else
					#endif
					{
						popup.postError("Reported rate potentially unreliable, "
							"using the detected or default rate may give better results");
					}
				}
				onFrameTimeChange.callSafe(0);
				view.popAndShow();
			});
		multiChoiceView.setItem(1, "Set default rate",
			[this, vidSys](TextMenuItem &, View &view, Input::Event e)
			{
				onFrameTimeChange.callSafe(EmuSystem::defaultFrameTime(vidSys));
				view.popAndShow();
			});
		multiChoiceView.setItem(2, "Set custom rate",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				auto &textInputView = *new CollectTextInputView{view.window()};
				textInputView.init("Input decimal or fraction", "", getCollectTextCloseAsset());
				textInputView.onText() =
					[this](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							double numer, denom;
							int items = sscanf(str, "%lf /%lf", &numer, &denom);
							if(items == 1 && numer > 0)
							{
								onFrameTimeChange.callSafe(1. / numer);
							}
							else if(items > 1 && (numer > 0 && denom > 0))
							{
								onFrameTimeChange.callSafe(denom / numer);
							}
							else
							{
								popup.postError("Invalid input");
								return 1;
							}
						}
						view.dismiss();
						return 0;
					};
				view.popAndShow();
				modalViewController.pushAndShow(textInputView, e);
			});
		if(includeFrameRateDetection)
		{
			multiChoiceView.setItem(3, "Detect screen's rate and set",
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					auto &frView = *new DetectFrameRateView{view.window()};
					frView.init();
					frView.onDetectFrameTime =
						[this](double frameTime)
						{
							if(frameTime)
							{
								onFrameTimeChange.callSafe(frameTime);
							}
							else
							{
								popup.postError("Detected rate too unstable to use");
							}
						};
					view.popAndShow();
					modalViewController.pushAndShow(frView, e);
				});
		}
		viewStack.pushAndShow(multiChoiceView, e);
	}
} frameRateSelectMenu;

void FirmwarePathSelector::onClose(Input::Event e)
{
	snprintf(optionFirmwarePath, sizeof(FS::PathString), "%s", FS::current_path().data());
	logMsg("set firmware path %s", (char*)optionFirmwarePath);
	if(onPathChange) onPathChange(optionFirmwarePath);
	workDirStack.pop();
}

void FirmwarePathSelector::init(const char *name, Input::Event e)
{
	auto &multiChoiceView = *new MultiChoiceView{name, mainWin.win};
	multiChoiceView.init(2);
	multiChoiceView.setItem(0, "Set Custom Path",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			viewStack.popAndShow();
			workDirStack.push();
			FS::current_path(optionFirmwarePath);
			auto &fPicker = *new EmuFilePicker{mainWin.win};
			fPicker.init(true, {});
			fPicker.setOnClose(
				[this](FSPicker &picker, Input::Event e)
				{
					onClose(e);
					picker.dismiss();
				});
			modalViewController.pushAndShow(fPicker, e);
		});
	multiChoiceView.setItem(1, "Default",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			viewStack.popAndShow();
			strcpy(optionFirmwarePath, "");
			if(onPathChange) onPathChange("");
		});
	viewStack.pushAndShow(multiChoiceView, e);
}

template <size_t S>
static void printFrameRateStr(char (&str)[S])
{
	string_printf(str, "Frame Rate: %.2fHz",
		1. / EmuSystem::frameTime(EmuSystem::VIDSYS_NATIVE_NTSC));
}

template <size_t S>
static void printFrameRatePALStr(char (&str)[S])
{
	string_printf(str, "Frame Rate (PAL): %.2fHz",
		1. / EmuSystem::frameTime(EmuSystem::VIDSYS_PAL));
}

void OptionView::loadVideoItems(MenuItem *item[], uint &items)
{
	name_ = "Video Options";
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	frameIntervalInit(); item[items++] = &frameInterval;
	#endif
	dropLateFrames.init(optionSkipLateFrames); item[items++] = &dropLateFrames;
	if(!optionFrameRate.isConst)
	{
		printFrameRateStr(frameRateStr);
		frameRate.init(frameRateStr, true); item[items++] = &frameRate;
	}
	if(!optionFrameRatePAL.isConst)
	{
		printFrameRatePALStr(frameRatePALStr);
		frameRatePAL.init(frameRatePALStr, true); item[items++] = &frameRatePAL;
	}
	imgFilter.init(optionImgFilter); item[items++] = &imgFilter;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffectInit(); item[items++] = &imgEffect;
	#endif
	overlayEffectInit(); item[items++] = &overlayEffect;
	overlayEffectLevelInit(); item[items++] = &overlayEffectLevel;
	zoomInit(); item[items++] = &zoom;
	viewportZoomInit(); item[items++] = &viewportZoom;
	aspectRatioInit(); item[items++] = &aspectRatio;
	#ifdef CONFIG_BASE_ANDROID
	if(!Config::MACHINE_IS_OUYA)
	{
		androidTextureStorageInit(); item[items++] = &androidTextureStorage;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffectPixelFormatInit(); item[items++] = &imgEffectPixelFormat;
	#endif
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	windowPixelFormatInit(); item[items++] = &windowPixelFormat;
	#endif
	if(!optionDitherImage.isConst)
	{
		dither.init(optionDitherImage); item[items++] = &dither;
	}
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	secondDisplay.init(false); item[items++] = &secondDisplay;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	if(!optionShowOnSecondScreen.isConst)
	{
		showOnSecondScreen.init(optionShowOnSecondScreen); item[items++] = &showOnSecondScreen;
	}
	#endif
}

void OptionView::loadAudioItems(MenuItem *item[], uint &items)
{
	name_ = "Audio Options";
	snd.init(optionSound); item[items++] = &snd;
	if(!optionSoundRate.isConst) { audioRateInit(); item[items++] = &audioRate; }
	#ifdef CONFIG_AUDIO_LATENCY_HINT
	soundBuffersInit(); item[items++] = &soundBuffers;
	#endif
#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
	sndUnderrunCheck.init(optionSoundUnderrunCheck); item[items++] = &sndUnderrunCheck;
	#endif
	#ifdef CONFIG_AUDIO_SOLO_MIX
	audioSoloMix.init(!optionAudioSoloMix); item[items++] = &audioSoloMix;
	#endif
}

void OptionView::loadInputItems(MenuItem *item[], uint &items)
{
	name_ = "Input Options";
}

void OptionView::loadSystemItems(MenuItem *item[], uint &items)
{
	name_ = "System Options";
	autoSaveStateInit(); item[items++] = &autoSaveState;
	confirmAutoLoadState.init(optionConfirmAutoLoadState); item[items++] = &confirmAutoLoadState;
	confirmOverwriteState.init(optionConfirmOverwriteState); item[items++] = &confirmOverwriteState;
	printPathMenuEntryStr(savePathStr);
	savePath.init(savePathStr, true); item[items++] = &savePath;
	checkSavePathWriteAccess.init(optionCheckSavePathWriteAccess); item[items++] = &checkSavePathWriteAccess;
	fastForwardSpeedinit(); item[items++] = &fastForwardSpeed;
	#ifdef __ANDROID__
	processPriorityInit(); item[items++] = &processPriority;
	manageCPUFreq.init(optionManageCPUFreq); item[items++] = &manageCPUFreq;
	#endif
}

void OptionView::loadGUIItems(MenuItem *item[], uint &items)
{
	name_ = "GUI Options";
	if(!optionPauseUnfocused.isConst)
	{
		pauseUnfocused.init(optionPauseUnfocused); item[items++] = &pauseUnfocused;
	}
	if(!optionNotificationIcon.isConst)
	{
		notificationIcon.init(optionNotificationIcon); item[items++] = &notificationIcon;
	}
	if(!optionTitleBar.isConst)
	{
		navView.init(optionTitleBar); item[items++] = &navView;
	}
	if(!View::needsBackControlIsConst)
	{
		backNav.init(View::needsBackControl); item[items++] = &backNav;
	}
	rememberLastMenu.init(optionRememberLastMenu); item[items++] = &rememberLastMenu;
	if(!optionFontSize.isConst)
	{
		fontSizeInit(); item[items++] = &fontSize;
	}
	if(!optionIdleDisplayPowerSave.isConst)
	{
		idleDisplayPowerSave.init(optionIdleDisplayPowerSave); item[items++] = &idleDisplayPowerSave;
	}
	if(!optionLowProfileOSNav.isConst)
	{
		uiVisibiltyInit(optionLowProfileOSNav, lowProfileOSNav);
		item[items++] = &lowProfileOSNav;
	}
	if(!optionHideOSNav.isConst)
	{
		uiVisibiltyInit(optionHideOSNav, hideOSNav);
		item[items++] = &hideOSNav;
	}
	if(!optionHideStatusBar.isConst)
	{
		uiVisibiltyInit(optionHideStatusBar, statusBar); item[items++] = &statusBar;
	}
	if(EmuSystem::hasBundledGames)
	{
		showBundledGames.init(optionShowBundledGames); item[items++] = &showBundledGames;
	}
	if(!optionGameOrientation.isConst)
	{
		orientationHeading.init(); item[items++] = &orientationHeading;
		gameOrientationInit(); item[items++] = &gameOrientation;
		menuOrientationInit(); item[items++] = &menuOrientation;
	}
}

void OptionView::init(uint idx)
{
	uint i = 0;
	switch(idx)
	{
		bcase 0: loadVideoItems(item, i);
		bcase 1: loadAudioItems(item, i);
		bcase 2: loadInputItems(item, i);
		bcase 3: loadSystemItems(item, i);
		bcase 4: loadGUIItems(item, i);
	}
	assert(i <= sizeofArray(item));
	TableView::init(item, i);
}

OptionView::OptionView(Base::Window &win):
	TableView{"Options", win},
	// Video
	#ifdef __ANDROID__
	androidTextureStorage
	{
		"GPU Copy Mode",
		[this](MultiChoiceMenuItem &item, View &view, int val)
		{
			using namespace Gfx;
			static auto modeForMenuValue =
				[](int val)
				{
					switch(val)
					{
						case 1: return OPTION_ANDROID_TEXTURE_STORAGE_NONE;
						case 2: return OPTION_ANDROID_TEXTURE_STORAGE_GRAPHIC_BUFFER;
						case 3: return OPTION_ANDROID_TEXTURE_STORAGE_SURFACE_TEXTURE;
					}
					return OPTION_ANDROID_TEXTURE_STORAGE_AUTO;
				};
			static auto resetVideo =
				[]()
				{
					if(emuVideo.vidImg)
					{
						// texture may switch to external format so
						// force effect shaders to re-compile
						emuVideoLayer.setEffect(0);
						emuVideo.reinitImage();
						emuVideo.clearImage();
						#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
						emuVideoLayer.setEffect(optionImgEffect);
						#endif
					}
				};
			static auto setAutoMode =
				[](MultiChoiceSelectMenuItem &item, View &view)
				{
					item.updateVal(0, view);
					optionAndroidTextureStorage = OPTION_ANDROID_TEXTURE_STORAGE_AUTO;
					Gfx::Texture::setAndroidStorageImpl(Texture::ANDROID_AUTO);
					resetVideo();
				};
			static auto setMode =
				[](MultiChoiceSelectMenuItem &item, View &view)
				{
					uint8 newMode = modeForMenuValue(item.choice);
					if(!Gfx::Texture::setAndroidStorageImpl(makeAndroidStorageImpl(newMode)))
					{
						popup.postError("Not supported on this GPU, using Auto");
						setAutoMode(item, view);
					}
					else
					{
						resetVideo();
						optionAndroidTextureStorage = newMode;
						if(newMode == OPTION_ANDROID_TEXTURE_STORAGE_AUTO)
						{
							const char *modeStr = "Standard";
							switch(Texture::androidStorageImpl())
							{
								bcase Texture::ANDROID_GRAPHIC_BUFFER: modeStr = "Graphic Buffer";
								bcase Texture::ANDROID_SURFACE_TEXTURE: modeStr = "Surface Texture";
								bdefault: break;
							}
							popup.printf(3, false, "Set %s mode via Auto", modeStr);
						}
					}
				};
			uint8 newMode = modeForMenuValue(val);
			if(newMode == OPTION_ANDROID_TEXTURE_STORAGE_GRAPHIC_BUFFER &&
				!Gfx::Texture::isAndroidGraphicBufferStorageWhitelisted())
			{
				auto &ynAlertView = *new YesNoAlertView{view.window(),
					"Graphic Buffer improves performance but may hang or crash "
					"the app depending on your device or GPU. Really use this option?"};
				ynAlertView.onYes() =
					[this](Input::Event e)
					{
						setMode(androidTextureStorage, *this);
					};
				ynAlertView.onNo() =
					[this](Input::Event e)
					{
						setAutoMode(androidTextureStorage, *this);
					};
				modalViewController.pushAndShow(ynAlertView, Input::defaultEvent());
			}
			else
			{
				setMode(androidTextureStorage, *this);
			}
		}
	},
	#endif
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	frameInterval
	{
		"Target Frame Rate",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionFrameInterval.val = val;
			logMsg("set frame interval: %d", int(optionFrameInterval));
		}
	},
	#endif
	dropLateFrames
	{
		"Skip Late Frames",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionSkipLateFrames.val = item.on;
		}
	},
	frameRate
	{
		"",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			frameRateSelectMenu.init(EmuSystem::VIDSYS_NATIVE_NTSC, e);
			frameRateSelectMenu.onFrameTimeChange =
				[this](double time)
				{
					double wantedTime = time;
					if(!time)
					{
						wantedTime = emuWin->win.screen()->frameTime();
					}
					if(!EmuSystem::setFrameTime(EmuSystem::VIDSYS_NATIVE_NTSC, wantedTime))
					{
						popup.printf(4, true, "%.2fHz not in valid range", 1. / wantedTime);
						return;
					}
					EmuSystem::configFrameTime();
					optionFrameRate = time;
					printFrameRateStr(frameRateStr);
					frameRate.compile(projP);
				};
			postDraw();
		}
	},
	frameRatePAL
	{
		"",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			frameRateSelectMenu.init(EmuSystem::VIDSYS_PAL, e);
			frameRateSelectMenu.onFrameTimeChange =
				[this](double time)
				{
					double wantedTime = time;
					if(!time)
					{
						wantedTime = emuWin->win.screen()->frameTime();
					}
					if(!EmuSystem::setFrameTime(EmuSystem::VIDSYS_PAL, wantedTime))
					{
						popup.printf(4, true, "%.2fHz not in valid range", 1. / wantedTime);
						return;
					}
					EmuSystem::configFrameTime();
					optionFrameRatePAL = time;
					printFrameRatePALStr(frameRatePALStr);
					frameRatePAL.compile(projP);
				};
			postDraw();
		}
	},
	aspectRatio
	{
		"Aspect Ratio",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			optionAspectRatio.val = EmuSystem::aspectRatioInfo[val].aspect;
			logMsg("set aspect ratio: %u:%u", optionAspectRatio.val.x, optionAspectRatio.val.y);
			placeEmuViews();
			emuWin->win.postDraw();
		}
	},
	zoom
	{
		"Zoom",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			switch(val)
			{
				bcase 0: optionImageZoom.val = 100;
				bcase 1: optionImageZoom.val = 90;
				bcase 2: optionImageZoom.val = 80;
				bcase 3: optionImageZoom.val = 70;
				bcase 4: optionImageZoom.val = optionImageZoomIntegerOnly;
				bcase 5: optionImageZoom.val = optionImageZoomIntegerOnlyY;
			}
			logMsg("set image zoom: %d", int(optionImageZoom));
			placeEmuViews();
			emuWin->win.postDraw();
		}
	},
	viewportZoom
	{
		"Screen Area",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			switch(val)
			{
				bcase 0: optionViewportZoom.val = 100;
				bcase 1: optionViewportZoom.val = 95;
				bcase 2: optionViewportZoom.val = 90;
				bcase 3: optionViewportZoom.val = 85;
			}
			logMsg("set viewport zoom: %d", int(optionViewportZoom));
			startViewportAnimation(mainWin);
			window().postDraw();
		}
	},
	imgFilter
	{
		"Image Interpolation", "None", "Linear",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionImgFilter.val = item.on;
			emuVideoLayer.setLinearFilter(item.on);
			emuWin->win.postDraw();
		}
	},
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffect
	{
		"Image Effect",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			uint setVal = 0;
			switch(val)
			{
				bcase 1: setVal = VideoImageEffect::HQ2X;
				bcase 2: setVal = VideoImageEffect::SCALE2X;
				bcase 3: setVal = VideoImageEffect::PRESCALE2X;
			}
			optionImgEffect.val = setVal;
			if(emuVideo.vidImg)
			{
				emuVideoLayer.setEffect(setVal);
				emuWin->win.postDraw();
			}
		}
	},
	#endif
	overlayEffect
	{
		"Overlay Effect",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			uint setVal = 0;
			switch(val)
			{
				bcase 1: setVal = VideoImageOverlay::SCANLINES;
				bcase 2: setVal = VideoImageOverlay::SCANLINES_2;
				bcase 3: setVal = VideoImageOverlay::CRT;
				bcase 4: setVal = VideoImageOverlay::CRT_RGB;
				bcase 5: setVal = VideoImageOverlay::CRT_RGB_2;
			}
			optionOverlayEffect.val = setVal;
			emuVideoLayer.vidImgOverlay.setEffect(setVal);
			emuVideoLayer.placeOverlay();
			emuWin->win.postDraw();
		}
	},
	overlayEffectLevel
	{
		"Overlay Effect Level",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			uint setVal = 10;
			switch(val)
			{
				bcase 1: setVal = 25;
				bcase 2: setVal = 33;
				bcase 3: setVal = 50;
				bcase 4: setVal = 66;
				bcase 5: setVal = 75;
				bcase 6: setVal = 100;
			}
			optionOverlayEffectLevel.val = setVal;
			emuVideoLayer.vidImgOverlay.intensity = setVal/100.;
			emuWin->win.postDraw();
		}
	},
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	imgEffectPixelFormat
	{
		"Effect Color Format",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			PixelFormatID format = PIXEL_NONE;
			switch(val)
			{
				bcase 1: format = PIXEL_RGB565;
				bcase 2: format = PIXEL_RGBA8888;
			}
			if(format == PIXEL_NONE)
				popup.printf(3, false, "Set RGB565 mode via Auto");
			optionImageEffectPixelFormat.val = format;
			emuVideoLayer.vidImgEffect.setBitDepth(format == PIXEL_RGBA8888 ? 32 : 16);
			emuWin->win.postDraw();
		}
	},
	#endif
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	windowPixelFormat
	{
		"Display Color Format",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			uint format = PIXEL_NONE;
			switch(val)
			{
				bcase 1: format = PIXEL_RGB565;
				bcase 2: format = PIXEL_RGB888;
			}
			if(format == PIXEL_NONE)
			{
				popup.printf(3, false, "Set %s mode via Auto, restart app for option to take effect",
					Base::Window::defaultPixelFormat() == PIXEL_RGB565 ? "RGB565" : "RGB888");
			}
			else
			{
				popup.post("Restart app for option to take effect");
			}
			optionWindowPixelFormat.val = format;
		}
	},
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	secondDisplay
	{
		"2nd Window (for testing only)",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			setEmuViewOnExtraWindow(item.on);
		}
	},
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	showOnSecondScreen
	{
		"External Screen", "OS Managed", "Game Content",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionShowOnSecondScreen = item.on;
			if(Base::Screen::screens() > 1)
				setEmuViewOnExtraWindow(item.on);
		}
	},
	#endif
	dither
	{
		"Dither Image",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionDitherImage = item.on;
			Gfx::setDither(item.on);
		}
	},
	// Audio
	snd
	{
		"Sound",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionSound = item.on;
			if(!item.on)
				Audio::closePcm();
		}
	},
	#ifdef CONFIG_AUDIO_LATENCY_HINT
	soundBuffers
	{
		"Buffer Size In Frames",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			if(Audio::isOpen())
				Audio::closePcm();
			optionSoundBuffers = val+OPTION_SOUND_BUFFERS_MIN;
		}
	},
	#endif
	audioRate
	{
		"Sound Rate",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			uint rate = 44100;
			switch(val)
			{
				bcase 0: rate = 22050;
				bcase 1: rate = 32000;
				bcase 3: rate = 48000;
			}
			if(rate != optionSoundRate)
			{
				optionSoundRate = rate;
				EmuSystem::configAudioPlayback();
			}
		}
	},
	#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
	sndUnderrunCheck
	{
		"Strict Underrun Check",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			if(Audio::isOpen())
				Audio::closePcm();
			item.toggle(*this);
			optionSoundUnderrunCheck = item.on;
		}
	},
	#endif
	#ifdef CONFIG_AUDIO_SOLO_MIX
	audioSoloMix
	{
		"Mix With Other Apps",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionAudioSoloMix = !item.on;
		}
	},
	#endif
	// System
	autoSaveState
	{
		"Auto-save State",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			switch(val)
			{
				bcase 0: optionAutoSaveState.val = 0;
				bcase 1: optionAutoSaveState.val = 1;
				bcase 2: optionAutoSaveState.val = 15;
				bcase 3: optionAutoSaveState.val = 30;
			}
			logMsg("set auto-savestate %d", optionAutoSaveState.val);
		}
	},
	confirmAutoLoadState
	{
		"Confirm Auto-load State",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionConfirmAutoLoadState = item.on;
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionConfirmOverwriteState = item.on;
		}
	},
	savePath
	{
		"",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pathSelectMenu.init(e);
			pathSelectMenu.onPathChange =
				[this](const char *newPath)
				{
					if(string_equal(newPath, optionSavePathDefaultToken))
					{
						auto path = EmuSystem::baseDefaultGameSavePath();
						popup.printf(4, false, "Default Save Path:\n%s", path.data());
					}
					printPathMenuEntryStr(savePathStr);
					savePath.compile(projP);
					EmuSystem::setupGameSavePath();
					EmuSystem::savePathChanged();
				};
			postDraw();
		}
	},
	checkSavePathWriteAccess
	{
		"Check Save Path Write Access",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionCheckSavePathWriteAccess = item.on;
		}
	},
	fastForwardSpeed
	{
		"Fast Forward Speed",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			optionFastForwardSpeed = val + MIN_FAST_FORWARD_SPEED;
		}
	},
	#if defined __ANDROID__
	processPriority
	{
		"Process Priority",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			if(val == 0)
				optionProcessPriority.val = 0;
			else if(val == 1)
				optionProcessPriority.val = -6;
			else if(val == 2)
				optionProcessPriority.val = -14;
			Base::setProcessPriority(optionProcessPriority);
		}
	},
	manageCPUFreq
	{
		"Manage Cpufreq Governor (needs root)",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			cpuFreq.reset();
			if(!item.on)
			{
				cpuFreq = std::make_unique<RootCpufreqParamSetter>();
				if(!(*cpuFreq))
				{
					cpuFreq.reset();
					optionManageCPUFreq = 0;
					popup.postError("Error setting up parameters & root shell");
					return;
				}
				popup.post("Enabling this option can improve performance if the app has root permission");
			}
			item.toggle(*this);
			optionManageCPUFreq = item.on;
		}
	},
	#endif
	// GUI
	pauseUnfocused
	{
		Config::envIsPS3 ? "Pause in XMB" : "Pause if unfocused",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionPauseUnfocused = item.on;
		}
	},
	fontSize
	{
		"Font Size",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			optionFontSize = 3000;
			switch(val)
			{
				bcase 0: optionFontSize = 2000;
				bcase 1: optionFontSize = 2500;
				// 2: 3000
				bcase 3: optionFontSize = 3500;
				bcase 4: optionFontSize = 4000;
				bcase 5: optionFontSize = 4500;
				bcase 6: optionFontSize = 5000;
				bcase 7: optionFontSize = 5500;
				bcase 8: optionFontSize = 6000;
				bcase 9: optionFontSize = 6500;
				bcase 10: optionFontSize = 7000;
				bcase 11: optionFontSize = 7500;
				bcase 12: optionFontSize = 8000;
				bcase 13: optionFontSize = 8500;
				bcase 14: optionFontSize = 9000;
				bcase 15: optionFontSize = 9500;
				bcase 16: optionFontSize = 10000;
				bcase 17: optionFontSize = 10500;
			}
			setupFont();
			placeElements();
		}
	},
	notificationIcon
	{
		"Suspended App Icon",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionNotificationIcon = item.on;
		}
	},
	statusBar
	{
		"Hide Status Bar",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionHideStatusBar = val;
			applyOSNavStyle(false);
		}
	},
	lowProfileOSNav
	{
		"Dim OS Navigation",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionLowProfileOSNav = val;
			applyOSNavStyle(false);
		}
	},
	hideOSNav
	{
		"Hide OS Navigation",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionHideOSNav = val;
			applyOSNavStyle(false);
		}
	},
	idleDisplayPowerSave
	{
		"Dim Screen If Idle",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionIdleDisplayPowerSave = item.on;
			Base::setIdleDisplayPowerSave(item.on);
		}
	},
	navView
	{
		"Title Bar",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionTitleBar = item.on;
			viewStack.setNavView(item.on ? &viewNav : 0);
			placeElements();
		}
	},
	backNav
	{
		"Title Back Navigation",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			View::setNeedsBackControl(item.on);
			viewNav.setBackImage(View::needsBackControl ? &getAsset(ASSET_ARROW) : nullptr);
			placeElements();
		}
	},
	rememberLastMenu
	{
		"Remember Last Menu",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionRememberLastMenu = item.on;
		}
	},
	showBundledGames
	{
		"Show Bundled Games",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionShowBundledGames = item.on;
			mainMenu().deinit();
			initMainMenu(window());
		}
	},
	orientationHeading
	{
		"Orientation",
	},
	menuOrientation
	{
		"In Menu",
		[this](MultiChoiceMenuItem &, View &, int val)
		{
			optionMenuOrientation.val = convertOrientationMenuValueToOption(val);
			Gfx::setWindowValidOrientations(mainWin.win, optionMenuOrientation);
			logMsg("set menu orientation: %s", Base::orientationToStr(int(optionMenuOrientation)));
		}
	},
	gameOrientation
	{
		"In Game",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionGameOrientation.val = convertOrientationMenuValueToOption(val);
			logMsg("set game orientation: %s", Base::orientationToStr(int(optionGameOrientation)));
		}
	}
{}
