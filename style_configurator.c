
static int
style_button(struct nk_context* ctx, struct nk_style_button* out_style, struct nk_style_button** duplicate_styles, int n_dups)
{
	struct nk_style_button button = *out_style;
	//button = &style->button;
	//nk_zero_struct(*button);
	//button->normal          = nk_style_item_color(table[NK_COLOR_BUTTON]);
	//button->hover           = nk_style_item_color(table[NK_COLOR_BUTTON_HOVER]);
	//button->active          = nk_style_item_color(table[NK_COLOR_BUTTON_ACTIVE]);
	//button->border_color    = table[NK_COLOR_BORDER];
	//button->text_background = table[NK_COLOR_BUTTON];
	//button->text_normal     = table[NK_COLOR_TEXT];
	//button->text_hover      = table[NK_COLOR_TEXT];
	//button->text_active     = table[NK_COLOR_TEXT];
	//button->padding         = nk_vec2(2.0f,2.0f);
	//button->image_padding   = nk_vec2(0.0f,0.0f);
	//button->touch_padding   = nk_vec2(0.0f, 0.0f);
	//button->userdata        = nk_handle_ptr(0);
	//button->text_alignment  = NK_TEXT_CENTERED;
	//button->border          = 1.0f;
	//button->rounding        = 4.0f;
	//button->draw_begin      = 0;
	//button->draw_end        = 0;


	char buffer[64];

	// Assumes all the style items are colors not images
	nk_layout_row_dynamic(ctx, 30, 2);
	nk_label(ctx, "Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.normal.data.color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.hover.data.color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.active.data.color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Border:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.border_color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.border_color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.border_color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Background:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.text_background, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_background), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.text_background = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.text_normal, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_normal), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.text_normal = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.text_hover, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_hover), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.text_hover = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, button.text_active, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_active), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		button.text_active = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", button.padding.x, button.padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &button.padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &button.padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Image Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", button.image_padding.x, button.image_padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &button.image_padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &button.image_padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Touch Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", button.touch_padding.x, button.touch_padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &button.touch_padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &button.touch_padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}


	/*
enum nk_text_align {
NK_TEXT_ALIGN_LEFT        = 0x01,
NK_TEXT_ALIGN_CENTERED    = 0x02,
NK_TEXT_ALIGN_RIGHT       = 0x04,
NK_TEXT_ALIGN_TOP         = 0x08,
NK_TEXT_ALIGN_MIDDLE      = 0x10,
NK_TEXT_ALIGN_BOTTOM      = 0x20
};
enum nk_text_alignment {
NK_TEXT_LEFT        = NK_TEXT_ALIGN_MIDDLE|NK_TEXT_ALIGN_LEFT,
NK_TEXT_CENTERED    = NK_TEXT_ALIGN_MIDDLE|NK_TEXT_ALIGN_CENTERED,
NK_TEXT_RIGHT       = NK_TEXT_ALIGN_MIDDLE|NK_TEXT_ALIGN_RIGHT
};
*/
	// TODO support combining with TOP/MIDDLE/BOTTOM .. separate combo?
	const char* alignments[] = { "LEFT", "CENTERED", "RIGHT" };
	int aligns[3] = { NK_TEXT_LEFT, NK_TEXT_CENTERED, NK_TEXT_RIGHT };
	int cur_align;
	if (button.text_alignment == NK_TEXT_LEFT) {
		cur_align = 0;
	} else if (button.text_alignment == NK_TEXT_CENTERED) {
		cur_align = 1;
	} else {
		cur_align = 2;
	}
	nk_label(ctx, "Text Alignment:", NK_TEXT_LEFT);
	cur_align = nk_combo(ctx, alignments, NK_LEN(alignments), cur_align, 25, nk_vec2(200,200));
	button.text_alignment = aligns[cur_align];

	nk_property_float(ctx, "#Border:", -100.0f, &button.border, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Rounding:", -100.0f, &button.rounding, 100.0f, 1,0.5f);

	//
	// optional user callback stuff
	//button->userdata        = nk_handle_ptr(0);
	//button->draw_begin      = 0;
	//button->draw_end        = 0;


	*out_style = button;
	if (duplicate_styles) {
		for (int i=0; i<n_dups; ++i) {
			*duplicate_styles[i] = button;
		}
	}

}



static int
style_configurator(struct nk_context *ctx)
{
	/* window flags */
	int show_menu = nk_true;
	int titlebar = nk_true;
	int border = nk_true;
	int resize = nk_true;
	int movable = nk_true;
	int no_scrollbar = nk_false;
	int scale_left = nk_false;
	nk_flags window_flags = 0;
	int minimizable = nk_true;


	/* window flags */
	window_flags = 0;
	//ctx->style.window.header.align = header_align;
	if (border) window_flags |= NK_WINDOW_BORDER;
	if (resize) window_flags |= NK_WINDOW_SCALABLE;
	if (movable) window_flags |= NK_WINDOW_MOVABLE;
	if (no_scrollbar) window_flags |= NK_WINDOW_NO_SCROLLBAR;
	if (scale_left) window_flags |= NK_WINDOW_SCALE_LEFT;
	if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;


	struct nk_style *style = &ctx->style;

	struct nk_style_text text = style->text;
	struct nk_style_toggle *toggle;
	struct nk_style_selectable *select;
	struct nk_style_slider *slider;
	struct nk_style_progress *prog;
	struct nk_style_scrollbar *scroll;
	struct nk_style_edit *edit;
	struct nk_style_property *property;
	struct nk_style_combo *combo;
	struct nk_style_chart *chart;
	struct nk_style_tab *tab;
	struct nk_style_window *win;

	// TODO allow external table
	const struct nk_color* table = nk_default_color_style;



	if (nk_begin(ctx, "Configurator", nk_rect(10, 10, 400, 600), window_flags))
	{

		if (nk_tree_push(ctx, NK_TREE_TAB, "Text", NK_MINIMIZED)) {
			//text = &style->text;
			//text->color = table[NK_COLOR_TEXT];
			//text->padding = nk_vec2(0,0);

			char buffer[64];

			nk_layout_row_dynamic(ctx, 30, 2);
			nk_label(ctx, "Color:", NK_TEXT_LEFT);
			if (nk_combo_begin_color(ctx, text.color, nk_vec2(nk_widget_width(ctx), 400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(text.color), NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
				colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
				colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
				colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

				text.color = nk_rgb_cf(colorf);

				nk_combo_end(ctx);
			}

			nk_label(ctx, "Padding:", NK_TEXT_LEFT);
			sprintf(buffer, "%.2f, %.2f", text.padding.x, text.padding.y);
			if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
				nk_layout_row_dynamic(ctx, 25, 1);
				nk_property_float(ctx, "#X:", -100.0f, &text.padding.x, 100.0f, 1,0.5f);
				nk_property_float(ctx, "#Y:", -100.0f, &text.padding.y, 100.0f, 1,0.5f);
				nk_combo_end(ctx);
			}
			style->text = text;

			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Button", NK_MINIMIZED)) {
			style_button(ctx, &style->button, NULL, 0);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Contextual Button", NK_MINIMIZED)) {
			style_button(ctx, &style->contextual_button, NULL, 0);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Menu Button", NK_MINIMIZED)) {
			style_button(ctx, &style->menu_button, NULL, 0);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Slider Buttons", NK_MINIMIZED)) {
			struct nk_style_button* dups[1] = { &style->slider.dec_button };
			style_button(ctx, &style->slider.inc_button, dups, 1);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Scrollbar Buttons", NK_MINIMIZED)) {
			struct nk_style_button* dups[3] = { &style->scrollh.dec_button,
			                                          &style->scrollv.inc_button,
			                                          &style->scrollv.dec_button };
			style_button(ctx, &style->scrollh.inc_button, dups, 3);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Property Buttons", NK_MINIMIZED)) {
			struct nk_style_button* dups[1] = { &style->property.dec_button };
			style_button(ctx, &style->property.inc_button, dups, 1);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Combo Buttons", NK_MINIMIZED)) {
			style_button(ctx, &style->combo.button, NULL, 0);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Tab Min/Max Buttons", NK_MINIMIZED)) {
			struct nk_style_button* dups[1] = { &style->tab.tab_maximize_button };
			style_button(ctx, &style->tab.tab_minimize_button, dups, 1);
			nk_tree_pop(ctx);
		}
		if (nk_tree_push(ctx, NK_TREE_TAB, "Nobe Min/Max Buttons", NK_MINIMIZED)) {
			struct nk_style_button* dups[1] = { &style->tab.node_maximize_button };
			style_button(ctx, &style->tab.node_minimize_button, dups, 1);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Window Header Close Buttons", NK_MINIMIZED)) {
			style_button(ctx, &style->window.header.close_button, NULL, 0);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Window Header Minimize Buttons", NK_MINIMIZED)) {
			style_button(ctx, &style->window.header.minimize_button, NULL, 0);
			nk_tree_pop(ctx);
		}
	}


//
//
//		/* checkbox toggle */
//		toggle = &style->checkbox;
//		nk_zero_struct(*toggle);
//		toggle->normal          = nk_style_item_color(table[NK_COLOR_TOGGLE]);
//		toggle->hover           = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
//		toggle->active          = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
//		toggle->cursor_normal   = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
//		toggle->cursor_hover    = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
//		toggle->userdata        = nk_handle_ptr(0);
//		toggle->text_background = table[NK_COLOR_WINDOW];
//		toggle->text_normal     = table[NK_COLOR_TEXT];
//		toggle->text_hover      = table[NK_COLOR_TEXT];
//		toggle->text_active     = table[NK_COLOR_TEXT];
//		toggle->padding         = nk_vec2(2.0f, 2.0f);
//		toggle->touch_padding   = nk_vec2(0,0);
//		toggle->border_color    = nk_rgba(0,0,0,0);
//		toggle->border          = 0.0f;
//		toggle->spacing         = 4;
//
//		/* option toggle */
//		toggle = &style->option;
//		nk_zero_struct(*toggle);
//		toggle->normal          = nk_style_item_color(table[NK_COLOR_TOGGLE]);
//		toggle->hover           = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
//		toggle->active          = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
//		toggle->cursor_normal   = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
//		toggle->cursor_hover    = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
//		toggle->userdata        = nk_handle_ptr(0);
//		toggle->text_background = table[NK_COLOR_WINDOW];
//		toggle->text_normal     = table[NK_COLOR_TEXT];
//		toggle->text_hover      = table[NK_COLOR_TEXT];
//		toggle->text_active     = table[NK_COLOR_TEXT];
//		toggle->padding         = nk_vec2(3.0f, 3.0f);
//		toggle->touch_padding   = nk_vec2(0,0);
//		toggle->border_color    = nk_rgba(0,0,0,0);
//		toggle->border          = 0.0f;
//		toggle->spacing         = 4;
//
//		/* selectable */
//		select = &style->selectable;
//		nk_zero_struct(*select);
//		select->normal          = nk_style_item_color(table[NK_COLOR_SELECT]);
//		select->hover           = nk_style_item_color(table[NK_COLOR_SELECT]);
//		select->pressed         = nk_style_item_color(table[NK_COLOR_SELECT]);
//		select->normal_active   = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
//		select->hover_active    = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
//		select->pressed_active  = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
//		select->text_normal     = table[NK_COLOR_TEXT];
//		select->text_hover      = table[NK_COLOR_TEXT];
//		select->text_pressed    = table[NK_COLOR_TEXT];
//		select->text_normal_active  = table[NK_COLOR_TEXT];
//		select->text_hover_active   = table[NK_COLOR_TEXT];
//		select->text_pressed_active = table[NK_COLOR_TEXT];
//		select->padding         = nk_vec2(2.0f,2.0f);
//		select->image_padding   = nk_vec2(2.0f,2.0f);
//		select->touch_padding   = nk_vec2(0,0);
//		select->userdata        = nk_handle_ptr(0);
//		select->rounding        = 0.0f;
//		select->draw_begin      = 0;
//		select->draw_end        = 0;
//
//		/* slider */
//		slider = &style->slider;
//		nk_zero_struct(*slider);
//		slider->normal          = nk_style_item_hide();
//		slider->hover           = nk_style_item_hide();
//		slider->active          = nk_style_item_hide();
//		slider->bar_normal      = table[NK_COLOR_SLIDER];
//		slider->bar_hover       = table[NK_COLOR_SLIDER];
//		slider->bar_active      = table[NK_COLOR_SLIDER];
//		slider->bar_filled      = table[NK_COLOR_SLIDER_CURSOR];
//		slider->cursor_normal   = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR]);
//		slider->cursor_hover    = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_HOVER]);
//		slider->cursor_active   = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_ACTIVE]);
//		slider->inc_symbol      = NK_SYMBOL_TRIANGLE_RIGHT;
//		slider->dec_symbol      = NK_SYMBOL_TRIANGLE_LEFT;
//		slider->cursor_size     = nk_vec2(16,16);
//		slider->padding         = nk_vec2(2,2);
//		slider->spacing         = nk_vec2(2,2);
//		slider->userdata        = nk_handle_ptr(0);
//		slider->show_buttons    = nk_false;
//		slider->bar_height      = 8;
//		slider->rounding        = 0;
//		slider->draw_begin      = 0;
//		slider->draw_end        = 0;
//
//		/* progressbar */
//		prog = &style->progress;
//		nk_zero_struct(*prog);
//		prog->normal            = nk_style_item_color(table[NK_COLOR_SLIDER]);
//		prog->hover             = nk_style_item_color(table[NK_COLOR_SLIDER]);
//		prog->active            = nk_style_item_color(table[NK_COLOR_SLIDER]);
//		prog->cursor_normal     = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR]);
//		prog->cursor_hover      = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_HOVER]);
//		prog->cursor_active     = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_ACTIVE]);
//		prog->border_color      = nk_rgba(0,0,0,0);
//		prog->cursor_border_color = nk_rgba(0,0,0,0);
//		prog->userdata          = nk_handle_ptr(0);
//		prog->padding           = nk_vec2(4,4);
//		prog->rounding          = 0;
//		prog->border            = 0;
//		prog->cursor_rounding   = 0;
//		prog->cursor_border     = 0;
//		prog->draw_begin        = 0;
//		prog->draw_end          = 0;
//
//		/* scrollbars */
//		scroll = &style->scrollh;
//		nk_zero_struct(*scroll);
//		scroll->normal          = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
//		scroll->hover           = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
//		scroll->active          = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
//		scroll->cursor_normal   = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR]);
//		scroll->cursor_hover    = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR_HOVER]);
//		scroll->cursor_active   = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE]);
//		scroll->dec_symbol      = NK_SYMBOL_CIRCLE_SOLID;
//		scroll->inc_symbol      = NK_SYMBOL_CIRCLE_SOLID;
//		scroll->userdata        = nk_handle_ptr(0);
//		scroll->border_color    = table[NK_COLOR_SCROLLBAR];
//		scroll->cursor_border_color = table[NK_COLOR_SCROLLBAR];
//		scroll->padding         = nk_vec2(0,0);
//		scroll->show_buttons    = nk_false;
//		scroll->border          = 0;
//		scroll->rounding        = 0;
//		scroll->border_cursor   = 0;
//		scroll->rounding_cursor = 0;
//		scroll->draw_begin      = 0;
//		scroll->draw_end        = 0;
//		style->scrollv = style->scrollh;
//
//		/* edit */
//		edit = &style->edit;
//		nk_zero_struct(*edit);
//		edit->normal            = nk_style_item_color(table[NK_COLOR_EDIT]);
//		edit->hover             = nk_style_item_color(table[NK_COLOR_EDIT]);
//		edit->active            = nk_style_item_color(table[NK_COLOR_EDIT]);
//		edit->cursor_normal     = table[NK_COLOR_TEXT];
//		edit->cursor_hover      = table[NK_COLOR_TEXT];
//		edit->cursor_text_normal= table[NK_COLOR_EDIT];
//		edit->cursor_text_hover = table[NK_COLOR_EDIT];
//		edit->border_color      = table[NK_COLOR_BORDER];
//		edit->text_normal       = table[NK_COLOR_TEXT];
//		edit->text_hover        = table[NK_COLOR_TEXT];
//		edit->text_active       = table[NK_COLOR_TEXT];
//		edit->selected_normal   = table[NK_COLOR_TEXT];
//		edit->selected_hover    = table[NK_COLOR_TEXT];
//		edit->selected_text_normal  = table[NK_COLOR_EDIT];
//		edit->selected_text_hover   = table[NK_COLOR_EDIT];
//		edit->scrollbar_size    = nk_vec2(10,10);
//		edit->scrollbar         = style->scrollv;
//		edit->padding           = nk_vec2(4,4);
//		edit->row_padding       = 2;
//		edit->cursor_size       = 4;
//		edit->border            = 1;
//		edit->rounding          = 0;
//
//		/* property */
//		property = &style->property;
//		nk_zero_struct(*property);
//		property->normal        = nk_style_item_color(table[NK_COLOR_PROPERTY]);
//		property->hover         = nk_style_item_color(table[NK_COLOR_PROPERTY]);
//		property->active        = nk_style_item_color(table[NK_COLOR_PROPERTY]);
//		property->border_color  = table[NK_COLOR_BORDER];
//		property->label_normal  = table[NK_COLOR_TEXT];
//		property->label_hover   = table[NK_COLOR_TEXT];
//		property->label_active  = table[NK_COLOR_TEXT];
//		property->sym_left      = NK_SYMBOL_TRIANGLE_LEFT;
//		property->sym_right     = NK_SYMBOL_TRIANGLE_RIGHT;
//		property->userdata      = nk_handle_ptr(0);
//		property->padding       = nk_vec2(4,4);
//		property->border        = 1;
//		property->rounding      = 10;
//		property->draw_begin    = 0;
//		property->draw_end      = 0;
//
//		/* property edit */
//		edit = &style->property.edit;
//		nk_zero_struct(*edit);
//		edit->normal            = nk_style_item_color(table[NK_COLOR_PROPERTY]);
//		edit->hover             = nk_style_item_color(table[NK_COLOR_PROPERTY]);
//		edit->active            = nk_style_item_color(table[NK_COLOR_PROPERTY]);
//		edit->border_color      = nk_rgba(0,0,0,0);
//		edit->cursor_normal     = table[NK_COLOR_TEXT];
//		edit->cursor_hover      = table[NK_COLOR_TEXT];
//		edit->cursor_text_normal= table[NK_COLOR_EDIT];
//		edit->cursor_text_hover = table[NK_COLOR_EDIT];
//		edit->text_normal       = table[NK_COLOR_TEXT];
//		edit->text_hover        = table[NK_COLOR_TEXT];
//		edit->text_active       = table[NK_COLOR_TEXT];
//		edit->selected_normal   = table[NK_COLOR_TEXT];
//		edit->selected_hover    = table[NK_COLOR_TEXT];
//		edit->selected_text_normal  = table[NK_COLOR_EDIT];
//		edit->selected_text_hover   = table[NK_COLOR_EDIT];
//		edit->padding           = nk_vec2(0,0);
//		edit->cursor_size       = 8;
//		edit->border            = 0;
//		edit->rounding          = 0;
//
//		/* chart */
//		chart = &style->chart;
//		nk_zero_struct(*chart);
//		chart->background       = nk_style_item_color(table[NK_COLOR_CHART]);
//		chart->border_color     = table[NK_COLOR_BORDER];
//		chart->selected_color   = table[NK_COLOR_CHART_COLOR_HIGHLIGHT];
//		chart->color            = table[NK_COLOR_CHART_COLOR];
//		chart->padding          = nk_vec2(4,4);
//		chart->border           = 0;
//		chart->rounding         = 0;
//
//		/* combo */
//		combo = &style->combo;
//		combo->normal           = nk_style_item_color(table[NK_COLOR_COMBO]);
//		combo->hover            = nk_style_item_color(table[NK_COLOR_COMBO]);
//		combo->active           = nk_style_item_color(table[NK_COLOR_COMBO]);
//		combo->border_color     = table[NK_COLOR_BORDER];
//		combo->label_normal     = table[NK_COLOR_TEXT];
//		combo->label_hover      = table[NK_COLOR_TEXT];
//		combo->label_active     = table[NK_COLOR_TEXT];
//		combo->sym_normal       = NK_SYMBOL_TRIANGLE_DOWN;
//		combo->sym_hover        = NK_SYMBOL_TRIANGLE_DOWN;
//		combo->sym_active       = NK_SYMBOL_TRIANGLE_DOWN;
//		combo->content_padding  = nk_vec2(4,4);
//		combo->button_padding   = nk_vec2(0,4);
//		combo->spacing          = nk_vec2(4,0);
//		combo->border           = 1;
//		combo->rounding         = 0;
//
//		/* tab */
//		tab = &style->tab;
//		tab->background         = nk_style_item_color(table[NK_COLOR_TAB_HEADER]);
//		tab->border_color       = table[NK_COLOR_BORDER];
//		tab->text               = table[NK_COLOR_TEXT];
//		tab->sym_minimize       = NK_SYMBOL_TRIANGLE_RIGHT;
//		tab->sym_maximize       = NK_SYMBOL_TRIANGLE_DOWN;
//		tab->padding            = nk_vec2(4,4);
//		tab->spacing            = nk_vec2(4,4);
//		tab->indent             = 10.0f;
//		tab->border             = 1;
//		tab->rounding           = 0;
//
//		/* window header */
//		win = &style->window;
//		win->header.align = NK_HEADER_RIGHT;
//		win->header.close_symbol = NK_SYMBOL_X;
//		win->header.minimize_symbol = NK_SYMBOL_MINUS;
//		win->header.maximize_symbol = NK_SYMBOL_PLUS;
//		win->header.normal = nk_style_item_color(table[NK_COLOR_HEADER]);
//		win->header.hover = nk_style_item_color(table[NK_COLOR_HEADER]);
//		win->header.active = nk_style_item_color(table[NK_COLOR_HEADER]);
//		win->header.label_normal = table[NK_COLOR_TEXT];
//		win->header.label_hover = table[NK_COLOR_TEXT];
//		win->header.label_active = table[NK_COLOR_TEXT];
//		win->header.label_padding = nk_vec2(4,4);
//		win->header.padding = nk_vec2(4,4);
//		win->header.spacing = nk_vec2(0,0);
//
//		/* window */
//		win->background = table[NK_COLOR_WINDOW];
//		win->fixed_background = nk_style_item_color(table[NK_COLOR_WINDOW]);
//		win->border_color = table[NK_COLOR_BORDER];
//		win->popup_border_color = table[NK_COLOR_BORDER];
//		win->combo_border_color = table[NK_COLOR_BORDER];
//		win->contextual_border_color = table[NK_COLOR_BORDER];
//		win->menu_border_color = table[NK_COLOR_BORDER];
//		win->group_border_color = table[NK_COLOR_BORDER];
//		win->tooltip_border_color = table[NK_COLOR_BORDER];
//		win->scaler = nk_style_item_color(table[NK_COLOR_TEXT]);
//
//		win->rounding = 0.0f;
//		win->spacing = nk_vec2(4,4);
//		win->scrollbar_size = nk_vec2(10,10);
//		win->min_size = nk_vec2(64,64);
//
//		win->combo_border = 1.0f;
//		win->contextual_border = 1.0f;
//		win->menu_border = 1.0f;
//		win->group_border = 1.0f;
//		win->tooltip_border = 1.0f;
//		win->popup_border = 1.0f;
//		win->border = 2.0f;
//		win->min_row_height_padding = 8;
//
//		win->padding = nk_vec2(4,4);
//		win->group_padding = nk_vec2(4,4);
//		win->popup_padding = nk_vec2(4,4);
//		win->combo_padding = nk_vec2(4,4);
//		win->contextual_padding = nk_vec2(4,4);
//		win->menu_padding = nk_vec2(4,4);
//		win->tooltip_padding = nk_vec2(4,4);
//	}
	nk_end(ctx);
	return !nk_window_is_closed(ctx, "Configurator");
}


