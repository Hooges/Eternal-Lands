#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <SDL_types.h>

#include "context_menu.h"
#include "elwindows.h"
#include "font.h"
#include "gamewin.h"
#include "gl_init.h"
#include "interface.h"
#ifdef NEW_SOUND
#include "sound.h"
#endif

//
//  Implements a simple context menu system using the standard el windows.
//  Intended to be a familiar GUI context menu activated via a right mouse click.
//  At its simplest, the user code just needs create a new context object 
//  providing a callback function for when an option is selected or using boolean
//	only options.  The Container class provides a creation/destruction/management/access
//  wrapper.
//  Author bluap/pjbroad March/April 2008
//
namespace cm
{
	//
	//  The context menu class.  Each manu uses a seperate object stored in
	//	the Container class.  One el window is used for all menus.  This class
	//	impliments the menu display, option hight-lighting, checkbox options and
	//	menu option selection.  It also stored size, colour and state information.
	//
	class Menu
	{
		public:
			Menu(const char *menu_list, int (*handler)(window_info *, int, int, int, int));
			int show(int cm_widget_id);
			int display(window_info *win, int mx, int my, Uint32 flags);
			int click(window_info *win, int mx, int my, Uint32 flags);
			int set(const char *menu_list, int (*handler)(window_info *, int, int, int, int));
			void set_pre_show_handler(void (*handler)(window_info *, int, int, int)) { pre_show_handler = handler; }
			int set_sizes(int border, int text_border, int line_sep, float zoom);
			int set_colour(size_t cm_id, enum CM_COLOUR_NAME colour_name, float r, float g, float b);
			int change_line(size_t line_index, const char *new_entry);
			int bool_line(size_t line_index, int *control_var);
			int grey_line(size_t line_index, bool is_grey);
			void show_lines(size_t my_id);

		private:
			int resize(void);
			int bool_box_size(void) const { return 15; }
			int border, text_border, line_sep;
			float zoom, bool_tick_width;
			int opened_mouse_x, opened_mouse_y;
			int (*handler)(window_info *, int, int, int, int);
			void (*pre_show_handler)(window_info *, int, int, int);
			int width, height;
			int selection;
			bool menu_has_bools;
			class Menu_Line
			{
				public:
					Menu_Line(std::string _text) : text(_text), control_var(0), is_grey(false), is_separator(false) {}
					std::string text;
					int *control_var;
					bool is_grey;
					bool is_separator;
			};
			std::vector<Menu_Line> menu_lines;
			class myRGB
			{
				public:
					myRGB(void) : r(0.0), g(0.0), b(0.0) {}
					void set(float _r, float _g, float _b) { r=_r; g=_g; b=_b; }
					float r, g, b;
			};
			myRGB highlight_top;
			myRGB highlight_bottom;
			myRGB text_colour;
			myRGB grey_colour;
	};


	//
	//  A simple container class for the context menus.
	//  Provides creation and destruction of context menus
	//  and any other 'across all menu' functions
	//  There can only be one instance!
	//
	class Container
	{
		public:
			Container(void);
			~Container(void);
			size_t create(const char *menu_list, int (*handler)(window_info *, int, int, int, int ));
			int destroy(size_t cm_id);
			int pre_show_check(Uint32 flags);
			void post_show_check(bool force);
			int show_if_active(int window_id);
			int show_direct(size_t cm_id, int window_id, int widget_id);
			int set(size_t cm_id, const char *menu_list, int (*handler)(window_info *, int, int, int, int)) { if (!valid(cm_id)) return 0; return menus[cm_id]->set(menu_list, handler); }
			int set_pre_show_handler(size_t cm_id, void (*handler)(window_info *, int, int, int)) { if (!valid(cm_id)) return 0; menus[cm_id]->set_pre_show_handler(handler); return 1; }
			int bool_line(size_t cm_id, size_t line_index, int *control_var) { if (!valid(cm_id)) return 0; return menus[cm_id]->bool_line(line_index, control_var); }
			int grey_line(size_t cm_id, size_t line_index, bool is_grey) { if (!valid(cm_id)) return 0; return menus[cm_id]->grey_line(line_index, is_grey); }
			int set_sizes(size_t cm_id, int border, int text_border, int line_sep, float zoom) { if (!valid(cm_id)) return 0; return menus[cm_id]->set_sizes(border, text_border, line_sep, zoom); }
			int set_colour(size_t cm_id, enum CM_COLOUR_NAME colour_name, float r, float g, float b) { if (!valid(cm_id)) return 0; return menus[cm_id]->set_colour(cm_id, colour_name, r, g, b); }
			int add_window(size_t cm_id, int window_id) { if (!valid(cm_id)) return 0; full_windows.insert(std::pair<int, size_t>(window_id, cm_id)); return 1; }
			int add_region(size_t cm_id, int window_id, int posx, int posy, int lenx, int leny) { if (!valid(cm_id)) return 0; window_regions.insert(std::pair<int, Region>(window_id, Region(cm_id, posx, posy, lenx, leny))); return 1; }
			int add_widget(size_t cm_id, int window_id, int widget_id) { if (!valid(cm_id)) return 0; window_widgets.insert(std::pair<int, Widget>(window_id, Widget(cm_id, widget_id))); return 1; }
			int remove_window(int window_id) {  if (full_windows.erase(window_id)) return 1; return 0;}
			int remove_regions(int window_id) { if (window_regions.erase(window_id)) return 1; return 0; }
			int remove_widget(int window_id, int widget_id);
			int get_active_window_id(void) const { return active_window_id; }
			int get_active_widget_id(void) const { return active_widget_id; }
			void showinfo(void);

		private:
			class Region	//  Wrapper for window region activation area.
			{
				public:
					Region(size_t _cm_id, int _pos_x, int _pos_y, int _len_x, int _len_y)
						: cm_id(_cm_id), pos_x(_pos_x), pos_y(_pos_y), len_x(_len_x), len_y(_len_y) {}
					size_t cm_id;
					int pos_x, pos_y, len_x, len_y;
			};
			typedef std::multimap<int, Region> REG_MM;
			REG_MM window_regions;
			class Widget	//  Wrapper for window widget activation area.
			{
				public:
					Widget(size_t _cm_id, int _widget_id)
						: cm_id(_cm_id), widget_id(_widget_id) {}
					size_t cm_id;
					int widget_id;
			};
			typedef std::multimap<int, Widget > WID_MM;
			WID_MM window_widgets;
			bool valid(size_t cm_id) const { return cm_id<menus.size() && menus[cm_id]; }
			int cm_window_id;
			int active_window_id;
			int active_widget_id;
			std::vector<Menu*> menus;
			std::map<int, size_t> full_windows; // <window_id, cm_id>
			bool menu_opened;
			static int instance_count;
	};


	// The one and only instance of the context menu container class.
	static Container container;
	int Container::instance_count = 0;


	// Generic el windows callback for clicks in a context menu.
	extern "C" int click_context_handler(window_info *win, int mx, int my, Uint32 flags)
	{
		Menu *thismenu = (Menu *)win->data;
		return thismenu->click(win, mx, my, flags);
	}


	// Generic el windows callback for displaying a context menu.
	extern "C" int display_context_handler(window_info *win, int mx, int my, Uint32 flags)
	{
		Menu *thismenu = (Menu *)win->data;
		return thismenu->display(win, mx, my, flags);
	}


	// safely get a window_info pointer from a window_id
	window_info * window_info_from_id(int window_id)
	{
		if ((window_id > -1) && (window_id < windows_list.num_windows))
			return &windows_list.window[window_id];
		else
			return NULL;
	}


	// constructor - create the context menu window and initialise containers
	Container::Container(void)
		: cm_window_id(-1), active_window_id(-1), active_widget_id(-1), menu_opened(false)
	{
		assert(instance_count++==0);
		int our_root_win = (windows_on_top) ?-1 :game_root_win;
		if ((cm_window_id = create_window("Context Menu", our_root_win, 0, 0, 0, 0, 0,
				ELW_USE_BACKGROUND|ELW_USE_BORDER|ELW_ALPHA_BORDER)) == -1)
			return;
		set_window_handler(cm_window_id, ELW_HANDLER_DISPLAY, (int (*)())&display_context_handler );
		set_window_handler(cm_window_id, ELW_HANDLER_CLICK, (int (*)())&click_context_handler );
		menus.resize(20,0);
	}


	// destructor - destory all instances of context menus and the context menu window
	Container::~Container(void)
	{
		for (size_t i=0; i<menus.size(); ++i)
		{
			if (menus[i])
			{
				delete menus[i];
				menus[i] = 0;
			}
		}
		destroy_window(cm_window_id);
	}


	// create a new context menu object
	size_t Container::create(const char *menu_list, int (*handler)(window_info *, int, int, int, int ))
	{
		Menu *temp = new Menu(menu_list, handler);
		// find an unused slot ...
		for (size_t i=0; i<menus.size(); ++i)
		{
			if (!menus[i])
			{
				menus[i] = temp;
				return i;
			}
		}
		// ... or create some more space
		size_t oldsize = menus.size();
		menus.resize(oldsize*2,0);
		menus[oldsize] = temp;
		return oldsize;
	}


	// destroy the specified context menu object, removing all the activation points too
	int Container::destroy(size_t cm_id)
	{
		if (!valid(cm_id))
			return 0;
		
		bool found_entry = false;

		// remove all full windows linked to this context menu
		do
		{
			found_entry = false;
  			for (std::map<int, size_t>::iterator itp = full_windows.begin(); itp != full_windows.end(); ++itp)
			{
				if (itp->second == cm_id)
				{
					full_windows.erase(itp);
					found_entry = true;
					break;
				}
			}
		} while (found_entry);


		// remove all regions linked to this context menu
		do
		{
 			found_entry = false;
 			for (REG_MM::iterator itp = window_regions.begin(); itp != window_regions.end(); ++itp)
			{
				if (itp->second.cm_id == cm_id)
				{
					window_regions.erase(itp);
					found_entry = true;
					break;
				}
			}
		} while (found_entry);


		// remove all widgets linked to this context menu
		do
		{
 			found_entry = false;
 			for (WID_MM::iterator itp = window_widgets.begin(); itp != window_widgets.end(); ++itp)
			{
				if (itp->second.cm_id == cm_id)
				{
					window_widgets.erase(itp);
					found_entry = true;
					break;
				}
			}
		} while (found_entry);

		delete menus[cm_id];
		menus[cm_id] = 0;
		return 1;
		
	} // end Container::destroy()


	// do the pre show checks and return the activation state, e.g. if mouse right-clicked
	int Container::pre_show_check(Uint32 flags)
	{
		int cm_to_activate = flags & ELW_RIGHT_MOUSE;
		if (cm_to_activate && ((flags & SHIFT) || (flags & ALT) || (flags & CTRL)))
			cm_to_activate = 0;  // exclude right clicks with modifier keys pressed
		menu_opened = false;
		return cm_to_activate;
	}


	// hide the window if its's open and reset the active window/widget values
	void Container::post_show_check(bool force)
	{
		// don't close a just (show_direct()) opened context menu
		if (!force && menu_opened)
			select_window(cm_window_id);
		else
		{
			hide_window(cm_window_id);
			active_window_id = active_widget_id = -1;
		}
		menu_opened = false;
	}


	// check if the specified window has any activation points then open the relevant context menu
	int Container::show_if_active(int window_id)
	{
		// check we're in the specified window
		if (mouse_in_window(window_id, mouse_x, mouse_y) < 1)
			return 0;

		// check if this window has any widget activation settings and open if mouse in that widget
		{
  			std::pair< WID_MM::iterator, WID_MM::iterator> itp = window_widgets.equal_range(window_id);
  			for (WID_MM::iterator it = itp.first; it != itp.second; ++it)
			{
				window_info *win = window_info_from_id(window_id);
				widget_list *wid = widget_find(window_id, it->second.widget_id);
				assert(wid!=NULL || win!=NULL);
				if ((mouse_x > win->cur_x + wid->pos_x) && (mouse_x <= win->cur_x + wid->pos_x + wid->len_x) &&
		    		(mouse_y > win->cur_y + wid->pos_y) && (mouse_y <= win->cur_y + wid->pos_y + wid->len_y))
					return show_direct(it->second.cm_id, window_id, it->second.widget_id);
			}
		}

		// check if this window has any active regions and open if mouse in that region
		{
  			std::pair< REG_MM::iterator, REG_MM::iterator> itp = window_regions.equal_range(window_id);
  			for (REG_MM::iterator it = itp.first; it != itp.second; ++it)
			{
				window_info *win = window_info_from_id(window_id);
				assert(win!=NULL);
				Region *cr = &it->second;
				if ((mouse_x > win->cur_x + cr->pos_x) && (mouse_x < win->cur_x + cr->pos_x + cr->len_x) &&
		    		(mouse_y > win->cur_y + cr->pos_y) && (mouse_y < win->cur_y + cr->pos_y + cr->len_y))
					return show_direct(cr->cm_id, window_id, -1);
			}
		}

		// check if this window has a full-window context menu - do this last to allow others within window
		std::map<int,size_t>::iterator fwit = full_windows.find(window_id);
		if (fwit != full_windows.end())
			return show_direct(fwit->second, window_id, -1);

		return 0;

	} // end Container::show_if_active()

	
	// directly open the specified conetext menu, use specified window/widget id as if activated
	int Container::show_direct(size_t cm_id, int window_id, int widget_id)
	{
		if (!valid(cm_id))
			return 0;
		menu_opened = true;
		active_window_id = window_id;
		active_widget_id = widget_id;
		return menus[cm_id]->show(cm_window_id);
	}

	
	// remove any activation for the specifed window/widget
	int Container::remove_widget(int window_id, int widget_id)
	{
  		std::pair< WID_MM::iterator, WID_MM::iterator> itp = window_widgets.equal_range(window_id);
  		for (WID_MM::iterator it = itp.first; it != itp.second; ++it)
		{
			if (it->second.widget_id == widget_id)
			{
				window_widgets.erase(it);
				return 1;
			}
		}
		return 0;
	}
	

	// for debug - display info on status of container object
	void Container::showinfo(void)
	{
		std::cout << "\nContext menu information:" << std::endl;
		
		std::cout << "\nMenus:-" << std::endl;
		for (size_t i=0; i<menus.size(); ++i)
			if (menus[i])
				menus[i]->show_lines(i);
		
		std::cout << "\nFull Windows:-" << std::endl;
		for (std::map<int, size_t>::iterator itp = full_windows.begin(); itp != full_windows.end(); ++itp)
		{
			window_info *win = window_info_from_id(itp->first);
			std::cout << "  window_id=" << itp->first << " name=[" << ((win!=NULL)?win->window_name:"")
					  << "] menu_id=" << itp->second << std::endl;
		}
		
		std::cout << "\nWindow Regions:-" << std::endl;
		for (REG_MM::iterator itp = window_regions.begin(); itp != window_regions.end(); ++itp)
		{
			window_info *win = window_info_from_id(itp->first);
			std::cout << "  window_id=" << itp->first << " name=[" << ((win!=NULL)?win->window_name:"")
					  << "] region=(" << itp->second.pos_x << ", " << itp->second.pos_y << ", " << itp->second.len_x
					  << ", " << itp->second.len_y << ")" << " menu_id=" << itp->second.cm_id << std::endl;
		}
		
		std::cout << "\nWindow Widgets:-" << std::endl;
		for (WID_MM::iterator itp = window_widgets.begin(); itp != window_widgets.end(); ++itp)
		{
			window_info *win = window_info_from_id(itp->first);
			std::cout << "  window_id=" << itp->first << " name=[" << ((win!=NULL)?win->window_name:"")
					  << "] widget=" << itp->second.widget_id << " menu_id=" << itp->second.cm_id << std::endl;
		}
		
	} // end showinfo()
	
	
	
	
	
	// Constructor - set default values for new menu
	Menu::Menu(const char *menu_list, int (*handler)(window_info *, int, int, int, int))
		: border(5), text_border(5), line_sep(3), zoom(0.8), selection(-1), menu_has_bools(false)
	{
		set(menu_list, handler);
		pre_show_handler = 0;
		highlight_top.set(0.11f, 0.11f, 0.11f);
		highlight_bottom.set(0.77f, 0.57f, 0.39f);
		text_colour.set(1.0f, 1.0f, 1.0f);
		grey_colour.set(0.7f, 0.7f, 0.7f);
	}


	// Set or replace the menu items
	int Menu::set(const char *menu_list, int (*handler)(window_info *, int, int, int, int))
	{
		this->handler = handler;
		menu_lines.clear();
		std::string menu_string(menu_list);
		for (size_t pos = 0; pos != std::string::npos;)
		{
			size_t endpos = menu_string.find('\n', pos);
			std::string new_entry = menu_string.substr(pos, endpos-pos);
			if (!new_entry.empty())
			{
				Menu_Line newline(new_entry);
				if (new_entry == std::string("--"))
					newline.is_separator = true;
				menu_lines.push_back(newline);
			}
			if (endpos == std::string::npos)
				break;
			pos = endpos+1;
		}
		return resize();
	}


	//  Change a single menu item if it exists
	int Menu::change_line(size_t line_index, const char *new_entry)
	{
		if (line_index >= menu_lines.size())
			return 0;
		menu_lines[line_index].text = std::string(new_entry);
		menu_lines[line_index].control_var = 0;
		menu_lines[line_index].is_grey = menu_lines[line_index].is_separator = false;
		if (new_entry == std::string("--"))
			menu_lines[line_index].is_separator = true;
		return 1;
	}


	//  Make a line in the menu an off/on option, control_var determines state
	int Menu::bool_line(size_t line_index, int *control_var)
	{
		if (line_index >= menu_lines.size())
			return 0;
		menu_lines[line_index].control_var = control_var;
		if (!menu_has_bools)
		{
			menu_has_bools = true;
			resize();
		}
		return 1;
	}


	//  grey/ungrey a menu line
	int Menu::grey_line(size_t line_index, bool is_grey)
	{
		if (line_index >= menu_lines.size())
			return 0;
		menu_lines[line_index].is_grey = is_grey;
		return 1;
	}


	//  Calculate the height/width of the context menu and resize the window
	int Menu::resize(void)
	{
		const float scale = zoom * DEFAULT_FONT_X_LEN / 12.0;
		float fwidth = 0, fheight = 0;
		for (size_t i=0; i<menu_lines.size(); i++)
		{
			float str_width = 0;
			const char *thetext = menu_lines[i].text.c_str();
			while(*thetext != '\0')
				str_width += get_char_width(*thetext++) * scale;
			if (str_width > fwidth)
				fwidth = str_width;
			if (menu_lines[i].is_separator)
				fheight += zoom * DEFAULT_FONT_Y_LEN / 2;
			else
				fheight += zoom * DEFAULT_FONT_Y_LEN + line_sep;
		}
		bool_tick_width = (menu_has_bools)? (bool_box_size()+text_border)*zoom : 0;
		fwidth += bool_tick_width + (border + text_border) * 2;
		fheight += border * 2;
		height = int(fheight+0.5);
		width = int(fwidth+0.5);
		return 1;
	}


	//  show the selected context menu
	int Menu::show(int cm_window_id)
	{
		// if the window is already displayed, hide it then return
		if (get_show_window(cm_window_id))
		{
			hide_window(cm_window_id);
			return 1;
		}

		// save the mouse position for the callback
		int wx = opened_mouse_x = mouse_x;
		int wy = opened_mouse_y = mouse_y;

		// move the window, one corner anchored to the mouse click position, so its on screen
		if (wx+width > window_width)
			wx -= width;
   		if (wy+height > window_height)
			wy -= height;
		int our_root_win = (windows_on_top) ?-1 :game_root_win;
   		move_window(cm_window_id, our_root_win, 0, wx, wy);

		/* call any registered pre_show handler */
		if (pre_show_handler)
		{
			// if we have a parent window, the mouse position is the original position that opened the menu
			window_info *parent_win = window_info_from_id(container.get_active_window_id());
			if (parent_win != NULL)
			{
				int parent_win_x = opened_mouse_x - parent_win->cur_x;
				int parent_win_y = opened_mouse_y - parent_win->cur_y;
				(*pre_show_handler)(parent_win, container.get_active_widget_id(), parent_win_x, parent_win_y);
			}
			else
				(*pre_show_handler)(NULL, 0, 0, 0);
		}

		// make sure the window is updated with this instances size and data, then show it
		windows_list.window[cm_window_id].data = this;
		resize_window(cm_window_id, width, height);
		show_window(cm_window_id);
		select_window(cm_window_id);

		return 1;
	}


	//  display the menu options with highlight if mouse is over one
	int Menu::display(window_info *win, int mx, int my, Uint32 flags)
	{
		CHECK_GL_ERRORS();
		float currenty = border + line_sep;
		float line_step = line_sep + DEFAULT_FONT_Y_LEN * zoom;

		selection = -1;
		for (size_t i=0; i<menu_lines.size(); ++i)
		{
			// if the mouse is over a valid line, draw the highlight and select line
			if (!menu_lines[i].is_grey && !menu_lines[i].is_separator &&
			  (mouse_y > win->cur_y + currenty) &&
			  (mouse_y < win->cur_y + currenty + line_step) &&
			  (mouse_x > win->cur_x + border) &&
			  (mouse_x < win->cur_x + width - border))
			{
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_QUADS);
				glColor3f(highlight_top.r, highlight_top.g, highlight_top.b);	
				glVertex3i(border, int(currenty + 0.5) - line_sep, 0);
				glColor3f(highlight_bottom.r, highlight_bottom.g, highlight_bottom.b);
				glVertex3i(border, int(currenty + line_step + 0.5) - line_sep, 0);
				glVertex3i(border+width-2*border, int(currenty + line_step + 0.5) - line_sep, 0);
				glColor3f(highlight_top.r, highlight_top.g, highlight_top.b);	
				glVertex3i(border+width-2*border, int(currenty + 0.5) - line_sep, 0);
				glEnd();
				glEnable(GL_TEXTURE_2D);
				selection = i;
			}
			
			// draw a separator ...
			if (menu_lines[i].is_separator)
			{
				int posx = border + text_border;
				int posy = int(0.5 + currenty + DEFAULT_FONT_Y_LEN * zoom/4 - line_sep);
				glColor3f(grey_colour.r, grey_colour.g, grey_colour.b);
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_LINES);
				glVertex2i(posx, posy);
				glVertex2i(win->len_x - posx, posy);
				glEnd();
				glEnable(GL_TEXTURE_2D);
				currenty += DEFAULT_FONT_Y_LEN * zoom/2;
			}

			// ... or the menu line
			else
			{
				if (menu_lines[i].is_grey)
					glColor3f(grey_colour.r, grey_colour.g, grey_colour.b);
				else
					glColor3f(text_colour.r, text_colour.g, text_colour.b);

				// draw the tickbox if bool option */
				if (menu_has_bools && menu_lines[i].control_var)
				{
					int posx = border+text_border;
					int posy = int(currenty+0.5);
					int size = int(0.5 + bool_box_size() * zoom);
					glDisable(GL_TEXTURE_2D);
					glBegin( *menu_lines[i].control_var ? GL_QUADS: GL_LINE_LOOP);
					glVertex3i(posx, posy, 0);
					glVertex3i(posx + size, posy, 0);
					glVertex3i(posx + size, posy + size, 0);
					glVertex3i(posx, posy + size, 0);
					glEnd();
					glEnable(GL_TEXTURE_2D);
				}

				// draw the text
 				draw_string_zoomed(int(border+text_border+bool_tick_width+0.5), int(currenty+0.5), (unsigned char *)menu_lines[i].text.c_str(), 1, zoom);
				currenty += line_step;
			}
			
		} // end foir each line
		
		CHECK_GL_ERRORS();
		return 1;
		
	} // end Menu::display()


	//  if an option is selected, toggle if a bool option and call any callback function
	int Menu::click(window_info *win, int mx, int my, Uint32 flags)
	{
		if (selection < 0)
			return 0;

#ifdef NEW_SOUND
		add_sound_object(get_index_for_sound_type_name("Button Click"), 0, 0, 1);
#endif

		// if a bool line, toggle the control variable
		if (menu_lines[selection].control_var)
		{
			*menu_lines[selection].control_var ^= 1;
			if (!handler)
				return 1;
		}

		if (!handler)
			return 0;
			
		// if we have a parent window, the mouse position is the original position that opened the menu
		window_info *parent_win = window_info_from_id(container.get_active_window_id());
		if (parent_win != NULL)
		{
			int parent_win_x = opened_mouse_x - parent_win->cur_x;
			int parent_win_y = opened_mouse_y - parent_win->cur_y;
			return (*handler)(parent_win, container.get_active_widget_id(), parent_win_x, parent_win_y, selection);
		}
		else
			return (*handler)(NULL, 0, 0, 0, selection);
	}


	// set basic size properties
	int Menu::set_sizes(int border, int text_border, int line_sep, float zoom)
	{
		this->border = border;
		this->text_border = text_border;
		this->line_sep = line_sep;
		this->zoom = zoom;
		return resize();
	}
	
	// set the named property colour
	int Menu::set_colour(size_t cm_id, enum CM_COLOUR_NAME colour_name, float r, float g, float b)
	{
		switch (colour_name)
		{
			case CM_HIGHLIGHT_TOP: highlight_top.set(r,g,b); break;
			case CM_HIGHLIGHT_BOTTOM: highlight_bottom.set(r,g,b); break;
			case CM_TEXT: text_colour.set(r,g,b); break;
			case CM_GREY: grey_colour.set(r,g,b); break;
			default:
				return 0;
		}
		return 1;
	}
	
	
	// show information about menu lines
	void Menu::show_lines(size_t my_id)
	{
		std::cout << "  menu_id=" << my_id << " has_bools=" << menu_has_bools << std::endl;
		for (size_t i=0; i<menu_lines.size(); ++i)
		{
			if (menu_lines[i].control_var)
				std::cout << "    [" << menu_lines[i].text << "] value=" << *menu_lines[i].control_var;
			else
				std::cout << "    [" << menu_lines[i].text << "]";
			std::cout << " " << ((menu_lines[i].is_grey) ?"greyed":"ungreyed")
				  << " " << ((menu_lines[i].is_separator) ?"separator":"normal") << std::endl;
		}
	}
	

} // end cm namespace



// C wrapper functions
extern "C" size_t cm_create(const char *menu_list, int (*handler)(window_info *, int, int, int, int )) { return cm::container.create(menu_list, handler); }
extern "C" int cm_destroy(size_t cm_id) { return cm::container.destroy(cm_id); }
extern "C" int cm_pre_show_check(Uint32 flags) { return cm::container.pre_show_check(flags); }
extern "C" void cm_post_show_check(int force) { cm::container.post_show_check(force); }
extern "C" int cm_show_if_active(int window_id) { return cm::container.show_if_active(window_id); }
extern "C" int cm_show_direct(size_t cm_id, int window_id, int widget_id) { return cm::container.show_direct(cm_id, window_id, widget_id); }
extern "C" int cm_bool_line(size_t cm_id, size_t line_index, int *control_var) { return cm::container.bool_line(cm_id, line_index, control_var); }
extern "C" int cm_grey_line(size_t cm_id, size_t line_index, int is_grey) { return cm::container.grey_line(cm_id, line_index, is_grey); }
extern "C" int cm_set(size_t cm_id, const char *menu_list, int (*handler)(window_info *, int, int, int, int)) { return cm::container.set(cm_id, menu_list, handler); }
extern "C" int cm_set_pre_show_handler(size_t cm_id, void (*handler)(window_info *, int, int, int)) { return cm::container.set_pre_show_handler(cm_id, handler); }
extern "C" int cm_set_sizes(size_t cm_id, int border, int text_border, int line_sep, float zoom) { return cm::container.set_sizes(cm_id, border, text_border, line_sep, zoom); }
extern "C" int cm_set_colour(size_t cm_id, enum CM_COLOUR_NAME colour_name, float r, float g, float b) { return cm::container.set_colour(cm_id, colour_name, r, g, b); }
extern "C" int cm_add_window(size_t cm_id, int window_id) { return cm::container.add_window(cm_id, window_id); }
extern "C" int cm_add_region(size_t cm_id, int window_id, int posx, int posy, int lenx, int leny) { return cm::container.add_region(cm_id, window_id, posx, posy, lenx, leny); }
extern "C" int cm_add_widget(size_t cm_id, int window_id, int widget_id) { return cm::container.add_widget(cm_id, window_id, widget_id); }
extern "C" int cm_remove_window(int window_id) { return cm::container.remove_window(window_id); }
extern "C" int cm_remove_regions(int window_id) { return cm::container.remove_regions(window_id); }
extern "C" int cm_remove_widget(int window_id, int widget_id) { return cm::container.remove_widget(window_id, widget_id); }
extern "C" void cm_showinfo(void) { cm::container.showinfo(); }






//
//	Provides a window to exercise context menu functions.
//
#ifdef CONTEXT_MENUS_TEST

static int cm_test_win = -1;
static size_t cm_test_win_menu = -1;
static size_t cm_test_reg_menu = -1;
static size_t cm_test_wid_menu = -1;
static size_t cm_test_dir_menu = -1;
static int cm_test_wid = -1;
static int cm_bool_var = 0;
static int cm_coord[2][4] = {{200, 100, 40, 40}, {250, 150, 40, 40}};
static int cm_grey_var = 0;

static int cm_test_window_display_handler(window_info *win)
{
	static int last_bool_var = cm_bool_var;
	int i;
	CHECK_GL_ERRORS();
	// draw the region boxes so we know where they are
	for (i=0; i<2; i++)
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.77f, 0.57f, 0.39f);
		glBegin(GL_LINE_LOOP);
		glVertex3i(cm_coord[i][0], cm_coord[i][1], 0);
		glVertex3i(cm_coord[i][0]+cm_coord[i][2], cm_coord[i][1], 0);
		glVertex3i(cm_coord[i][0]+cm_coord[i][2], cm_coord[i][1]+cm_coord[i][3], 0);
		glVertex3i(cm_coord[i][0], cm_coord[i][1]+cm_coord[i][3], 0);
		glEnd();
		glEnable(GL_TEXTURE_2D);
	}
	if (cm_bool_var != last_bool_var)
	{
		last_bool_var = cm_bool_var;
		printf("Bool var changed, now=%d\n", cm_bool_var);
	}
	CHECK_GL_ERRORS();
	return 1;
}

static void cm_test_menu_pre_show_handler(window_info *win, int widget_id, int mx, int my)
{
	printf("CM pre show: Set bool menu cm_grey_line()=%d window_id=%d widget_id=%d mx=%d my=%d\n",
		cm_grey_line(cm_test_win_menu, 3, cm_grey_var), win->window_id, widget_id, mx, my );
}

static int cm_test_menu_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	printf("CM selected: window_id=%d widget_id=%d mx=%d my=%d option=%d\n", win->window_id, widget_id, mx, my, option );
	return 1;
}

static int cm_info_but_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	cm_showinfo();
	return 1;
}

static int cm_dir_click_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	if (flags & ELW_RIGHT_MOUSE)
		printf("Direct clicked cm_show_direct()=%d\n", cm_show_direct(cm_test_dir_menu, cm_test_win, -1));
	return 1;
}

static int cm_add_win_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	printf("Adding window cm_add_window()=%d\n", cm_add_window(cm_test_win_menu, widget->window_id));
	return 1;
}

extern "C" int cm_del_win_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	printf("Removing window cm_remove_window()=%d\n", cm_remove_window(widget->window_id));
	return 1;
}

static int cm_add_reg_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	int i;
	for (i=0; i<2; i++)
		printf("Adding region %d cm_add_region()=%d\n", i,
			cm_add_region(cm_test_reg_menu, widget->window_id, cm_coord[i][0], cm_coord[i][1], cm_coord[i][2], cm_coord[i][3] ));
	return 1;
}

extern "C" int cm_del_reg_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	printf("Removing regions status=%d\n", cm_remove_regions(widget->window_id));
	return 1;
}

static int cm_add_wid_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	printf("Adding widget cm_add_widget()=%d\n", cm_add_widget(cm_test_wid_menu, widget->window_id, cm_test_wid));
	return 1;
}

extern "C" int cm_del_wid_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	printf("Removing widget cm_remove_widget()=%d\n", cm_remove_widget(widget->window_id, cm_test_wid));
	return 1;
}

extern "C" int cm_test_window(char *text, int len)
{
	int cm_add_win = -1;
	int cm_add_reg = -1;
	int cm_add_wid = -1;
	int cm_del_win = -1;
	int cm_del_reg = -1;
	int cm_del_wid = -1;
	int cm_info_but = -1;
	int cm_dir_but = -1;
	
	if (cm_test_win == -1)
	{
		cm_test_win = create_window("Test Context Menu", -1, -1, 0, 0, 400, 400, ELW_WIN_DEFAULT);
		set_window_handler(cm_test_win, ELW_HANDLER_DISPLAY, (int (*)())&cm_test_window_display_handler );

		cm_add_win = button_add_extended(cm_test_win, 100, NULL, 10, 10, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Add window");
		cm_add_reg = button_add_extended(cm_test_win, 101, NULL, 10, 50, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Add region");
		cm_add_wid = button_add_extended(cm_test_win, 102, NULL, 10, 90, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Add widget");
		cm_del_win = button_add_extended(cm_test_win, 103, NULL, 10, 130, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Del window");
		cm_del_reg = button_add_extended(cm_test_win, 104, NULL, 10, 170, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Del regions");
		cm_del_wid = button_add_extended(cm_test_win, 105, NULL, 10, 210, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Del widget");
		cm_info_but = button_add_extended(cm_test_win, 106, NULL, 200, 10, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Show Info");
		cm_test_wid = button_add_extended(cm_test_win, 107, NULL, 200, 50, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Test Menu");
		cm_dir_but = button_add_extended(cm_test_win, 108, NULL, 200, 200, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, "Direct Menu");
		
		widget_set_OnClick(cm_test_win, cm_add_win, (int (*)())&cm_add_win_handler);
		widget_set_OnClick(cm_test_win, cm_del_win, (int (*)())&cm_del_win_handler);
		widget_set_OnClick(cm_test_win, cm_add_reg, (int (*)())&cm_add_reg_handler);
		widget_set_OnClick(cm_test_win, cm_del_reg, (int (*)())&cm_del_reg_handler);
		widget_set_OnClick(cm_test_win, cm_add_wid, (int (*)())&cm_add_wid_handler);
		widget_set_OnClick(cm_test_win, cm_del_wid, (int (*)())&cm_del_wid_handler);
		widget_set_OnClick(cm_test_win, cm_info_but, (int (*)())&cm_info_but_handler);
		widget_set_OnClick(cm_test_win, cm_dir_but, (int (*)())&cm_dir_click_handler);
		
		cm_test_win_menu = cm_create("", NULL);
		cm_test_reg_menu = cm_create("Region 1\nRegion 2\n", cm_test_menu_handler);
		cm_test_wid_menu = cm_create("Widget 1\n--\nWidget 3\nWidget 4\nWidget 5\nWidget 6\n", cm_test_menu_handler);
		cm_test_dir_menu = cm_create("Direct 1\nDirect 2\n", cm_test_menu_handler);
		printf("Created menus window=%d region=%d widget=%d direct=%d\n",
			cm_test_win_menu, cm_test_reg_menu, cm_test_wid_menu, cm_test_dir_menu);
		
		printf("Replacing window menu cm_set()=%d\n", cm_set(cm_test_win_menu, "Window 1\nWindow 2\n--\nGrey me...\nGrey above\n", cm_test_menu_handler));
		printf("Set win menu cm_bool_line()=%d\n", cm_bool_line(cm_test_win_menu, 0, &cm_bool_var));
		printf("Set win menu cm_bool_line()=%d\n", cm_bool_line(cm_test_win_menu, 1, &cm_bool_var));
		printf("Set win menu cm_grey_line()=%d\n", cm_grey_line(cm_test_win_menu, 3, cm_grey_var));
		printf("Set win menu cm_bool_line()=%d\n", cm_bool_line(cm_test_win_menu, 4, &cm_grey_var));
		printf("Set widget menu cm_grey_line()=%d\n", cm_grey_line(cm_test_wid_menu, 3, 1));
		printf("Calling cm_set_pre_show_handler()=%d\n", cm_set_pre_show_handler(cm_test_win_menu, cm_test_menu_pre_show_handler) );
		printf("Changing widget menu sizes cm_set_sizes()=%d\n", cm_set_sizes(cm_test_wid_menu, 10, 10, 10, 2));
		printf("Changing widget menu text colour cm_set_colour()=%d\n", cm_set_colour(cm_test_wid_menu, CM_TEXT, 0, 0, 1));
		printf("Changing widget menu grey colour cm_set_colour()=%d\n", cm_set_colour(cm_test_wid_menu, CM_GREY, 0, 0, 0.5));
		printf("Changing widget menu pale highlight colour cm_set_colour()=%d\n", cm_set_colour(cm_test_wid_menu, CM_HIGHLIGHT_TOP, 1, 0, 0));
		printf("Changing widget menu bright highlight colour cm_set_colour()=%d\n", cm_set_colour(cm_test_wid_menu, CM_HIGHLIGHT_BOTTOM, 0, 1, 0));
	}
	else
	{
		printf("\n\nDestroy cm_test_win_menu cm_destroy()=%d\n", cm_destroy(cm_test_win_menu));
		printf("Destroy cm_test_reg_menu cm_destroy()=%d\n", cm_destroy(cm_test_reg_menu));
		printf("Destroy cm_test_wid_menu cm_destroy()=%d\n", cm_destroy(cm_test_wid_menu));
		printf("Destroy cm_test_dir_menu cm_destroy()=%d\n", cm_destroy(cm_test_dir_menu));
		destroy_window(cm_test_win);
		cm_test_win = -1;
		cm_showinfo();
	}
	
	return 1;
}

#endif