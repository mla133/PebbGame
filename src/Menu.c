#include "pebble.h"

#include "Menu.h"
#include "UILayers.h"
#include "Utils.h"

bool MenuEntryIsActive(MenuEntry *entry)
{
	if(!entry)
		return false;

	return entry->text && entry->menuFunction;
}

static MenuDefinition *currentMenuDef = NULL;

//*************** Menu Windows ******************//

// I have a set of window structs here so that I can have only as many as needed 
// for the depth of stack, instead of one for each possible menu

typedef struct
{
	Window *window;
	bool inUse;
	MenuDefinition *menu;
} MenuWindow;

static MenuWindow menuWindows[MAX_MENU_WINDOWS];

void MenuInit(Window *window)
{
}

void MenuDeinit(Window *window)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "Menu DESTROY");
	MenuWindow *menuWindow = window_get_user_data(window);
	if(menuWindow)
	{
		menuWindow->inUse = false;
		menuWindow->menu = NULL;
	}
	window_destroy(window);
}

void MenuAppear(Window *window)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu Appear");
	int i;
	bool setSelected = false;
	MenuWindow *menuWindow = window_get_user_data(window);
	if(menuWindow)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu Exists in window");
		SetCurrentMenu(menuWindow->menu);
	}
		
	WindowAppear(window);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Window appear");
	if(!currentMenuDef)
	{
		HideAllMenuLayers();
		SetMenuDescription(NULL);
		return;
	}
	
	if(currentMenuDef->modify)
		currentMenuDef->modify(currentMenuDef->menuEntries);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Display menu entries");
	for(i = 0; i < MAX_MENU_ENTRIES; ++i)
	{
		MenuEntry *entry = &currentMenuDef->menuEntries[i];
		if(MenuEntryIsActive(entry))
		{
			ShowMenuLayer(i, entry->text);
			if(setSelected)
			{
				SetMenuHighlight(i, false);
			}
			else
			{
				SetMenuHighlight(i, true);
				setSelected = true;
				currentMenuDef->currentSelection = i;
				SetMenuDescription(entry->description);
			}
		}
		else
		{
			HideMenuLayer(i);
		}
	}
	
	if(menuWindow && menuWindow->menu && menuWindow->menu->mainImageId != -1)
	{
		LoadMainBmpImage(window, menuWindow->menu->mainImageId);
	}
}

void MenuDisappear(Window *window)
{
	WindowDisappear(window);
}

void SetCurrentMenu(MenuDefinition *menuDef)
{
	currentMenuDef = menuDef;
}

void PopMenu(void)
{
	window_stack_pop(currentMenuDef ? currentMenuDef->animated : true);
}
void CreateWindow(MenuWindow *newMenuWindow) {
	newMenuWindow->inUse = true;
	newMenuWindow->menu = currentMenuDef;

	newMenuWindow->window = InitializeMenuWindow("Menu", 
		currentMenuDef->init ? currentMenuDef->init : MenuInit,
		currentMenuDef->deinit ? currentMenuDef->deinit : MenuDeinit,
		currentMenuDef->appear ? currentMenuDef->appear : MenuAppear,
		currentMenuDef->disappear ? currentMenuDef->disappear : MenuDisappear);
		
	window_set_user_data(newMenuWindow->window,newMenuWindow);
	window_stack_push(newMenuWindow->window,currentMenuDef->animated);
}

void PushNewMenu(MenuDefinition *menuDef)
{
	SetCurrentMenu(menuDef);
	if(currentMenuDef)
	{
		int i = 0;

		MenuWindow *newMenuWindow = NULL;
		
		for(i = 0; i < MAX_MENU_WINDOWS; ++i)
		{
			if(!menuWindows[i].inUse)
			{	
				newMenuWindow = &menuWindows[i];
				break;
			}
		}
		
		if(!newMenuWindow)
		{
			window_stack_pop_all(true);
			return;
		}
		
		CreateWindow(newMenuWindow);
	
	}
}

// ******** CLICK **********//

void SelectSingleClickHandler(ClickRecognizerRef recognizer, Window *window)
{
	MenuEntry *currentEntry;
	if(!currentMenuDef)
		return;

	currentEntry = &currentMenuDef->menuEntries[currentMenuDef->currentSelection];
	if(!MenuEntryIsActive(currentEntry))
		return;

	currentEntry->menuFunction();
}

void IterateMenuEntries(int direction, int limit)
{
	int iterator,newSelection;

	if(!currentMenuDef)
		return;

	iterator = newSelection = currentMenuDef->currentSelection;
  
	while(iterator != limit)
	{
		MenuEntry *entry;
		iterator += direction;
		entry = &currentMenuDef->menuEntries[iterator];
		if(MenuEntryIsActive(entry))
		{
			newSelection = iterator;
			break;
		}
	}

	if(newSelection != currentMenuDef->currentSelection)
	{
		MenuEntry *entry = &currentMenuDef->menuEntries[newSelection];
		SetMenuHighlight(currentMenuDef->currentSelection, false);
		SetMenuHighlight(newSelection, true);
		currentMenuDef->currentSelection = newSelection;
		SetMenuDescription(entry->description);
	}
}

void UpSingleClickHandler(ClickRecognizerRef recognizer, Window *window)
{
	IterateMenuEntries(-1, 0);
}

void DownSingleClickHandler(ClickRecognizerRef recognizer, Window *window)
{
	IterateMenuEntries(1, MAX_MENU_ENTRIES-1);
}

#if OVERRIDE_BACK_BUTTON
void BackSingleClickHandler(ClickRecognizerRef recognizer, Window *window)
{
	if(currentMenuDef && currentMenuDef->disableBackButton)
		return;
		
	PopMenu();
}

void BackLongClickHandler(ClickRecognizerRef recognizer, Window *window)
{
	window_stack_pop_all(true);
}
#endif

void MenuClickConfigProvider(void *context)
{
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler)SelectSingleClickHandler);
	window_single_click_subscribe(BUTTON_ID_UP,(ClickHandler)UpSingleClickHandler);
	window_single_click_subscribe(BUTTON_ID_DOWN,(ClickHandler)DownSingleClickHandler);

	
#if OVERRIDE_BACK_BUTTON
	window_single_click_subscribe(BUTTON_ID_BACK, (ClickHandler)BackSingleClickHandler);
	window_long_click_subscribe(BUTTON_ID_BACK,500,(ClickHandler)BackLongClickHandler,NULL);
#endif
}

void SetMenuClickConfigProvider(Window *window)
{
	window_set_click_config_provider(window, (ClickConfigProvider) MenuClickConfigProvider);
}
