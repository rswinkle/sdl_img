
// TODO design decisions
// plural or not?  ie style_button or style_buttons?
// use the duplicate array method, or just yet the user
// manually set those after calling the function by accessing ctx->style->*?
//

// style_general? pass array in instead of static?
static void
style_colors(struct nk_context* ctx, struct nk_color color_table[NK_COLOR_COUNT])
{
	const char* color_labels[NK_COLOR_COUNT] =
	{
		"COLOR_TEXT:",
		"COLOR_WINDOW:",
		"COLOR_HEADER:",
		"COLOR_BORDER:",
		"COLOR_BUTTON:",
		"COLOR_BUTTON_HOVER:",
		"COLOR_BUTTON_ACTIVE:",
		"COLOR_TOGGLE:",
		"COLOR_TOGGLE_HOVER:",
		"COLOR_TOGGLE_CURSOR:",
		"COLOR_SELECT:",
		"COLOR_SELECT_ACTIVE:",
		"COLOR_SLIDER:",
		"COLOR_SLIDER_CURSOR:",
		"COLOR_SLIDER_CURSOR_HOVER:",
		"COLOR_SLIDER_CURSOR_ACTIVE:",
		"COLOR_PROPERTY:",
		"COLOR_EDIT:",
		"COLOR_EDIT_CURSOR:",
		"COLOR_COMBO:",
		"COLOR_CHART:",
		"COLOR_CHART_COLOR:",
		"COLOR_CHART_COLOR_HIGHLIGHT:",
		"COLOR_SCROLLBAR:",
		"COLOR_SCROLLBAR_CURSOR:",
		"COLOR_SCROLLBAR_CURSOR_HOVER:",
		"COLOR_SCROLLBAR_CURSOR_ACTIVE:",
		"COLOR_TAB_HEADER:"
	};

	nk_layout_row_dynamic(ctx, 30, 2);
	for (int i=0; i<NK_COLOR_COUNT; ++i) {
		nk_label(ctx, color_labels[i], NK_TEXT_LEFT);

		if (nk_combo_begin_color(ctx, color_table[i], nk_vec2(nk_widget_width(ctx), 400))) {
			nk_layout_row_dynamic(ctx, 120, 1);
			struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(color_table[i]), NK_RGB);
			nk_layout_row_dynamic(ctx, 25, 1);
			colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
			colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
			colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

			color_table[i] = nk_rgb_cf(colorf);

			nk_combo_end(ctx);
		}
	}

	nk_style_from_table(ctx, color_table);
}

static void
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.normal.data.color), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.hover.data.color), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.active.data.color), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.border_color), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_background), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_normal), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_hover), NK_RGB);
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
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(button.text_active), NK_RGB);
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

static void style_toggle(struct nk_context* ctx, struct nk_style_toggle* out_style)
{
	struct nk_style_toggle toggle = *out_style;
	//toggle = &style->checkbox;
	//nk_zero_struct(*toggle);
	//toggle->normal          = nk_style_item_color(table[NK_COLOR_TOGGLE]);
	//toggle->hover           = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
	//toggle->active          = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
	//toggle->cursor_normal   = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
	//toggle->cursor_hover    = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
	//toggle->userdata        = nk_handle_ptr(0);
	//toggle->text_background = table[NK_COLOR_WINDOW];
	//toggle->text_normal     = table[NK_COLOR_TEXT];
	//toggle->text_hover      = table[NK_COLOR_TEXT];
	//toggle->text_active     = table[NK_COLOR_TEXT];
	//toggle->padding         = nk_vec2(2.0f, 2.0f);
	//toggle->touch_padding   = nk_vec2(0,0);
	//toggle->border_color    = nk_rgba(0,0,0,0);
	//toggle->border          = 0.0f;
	//toggle->spacing         = 4;
	//

	char buffer[64];

	// Assumes all the style items are colors not images
	nk_layout_row_dynamic(ctx, 30, 2);

	nk_label(ctx, "Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.cursor_normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.cursor_normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.cursor_normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.cursor_hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.cursor_hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.cursor_hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Background:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.text_background, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.text_background), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.text_background = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.text_normal, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.text_normal), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.text_normal = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.text_hover, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.text_hover), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.text_hover = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.text_active, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.text_active), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.text_active = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", toggle.padding.x, toggle.padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &toggle.padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &toggle.padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Touch Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", toggle.touch_padding.x, toggle.touch_padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &toggle.touch_padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &toggle.touch_padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Border Color:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, toggle.border_color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(toggle.border_color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		toggle.border_color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_property_float(ctx, "#Border:", -100.0f, &toggle.border, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Spacing:", -100.0f, &toggle.spacing, 100.0f, 1,0.5f);
	

	*out_style = toggle;

}

static void
style_selectable(struct nk_context* ctx, struct nk_style_selectable* out_style)
{
	struct nk_style_selectable select = *out_style;
	//select = &style->selectable;
	//nk_zero_struct(*select);
	//select->normal          = nk_style_item_color(table[NK_COLOR_SELECT]);
	//select->hover           = nk_style_item_color(table[NK_COLOR_SELECT]);
	//select->pressed         = nk_style_item_color(table[NK_COLOR_SELECT]);
	//select->normal_active   = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
	//select->hover_active    = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
	//select->pressed_active  = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
	//select->text_normal     = table[NK_COLOR_TEXT];
	//select->text_hover      = table[NK_COLOR_TEXT];
	//select->text_pressed    = table[NK_COLOR_TEXT];
	//select->text_normal_active  = table[NK_COLOR_TEXT];
	//select->text_hover_active   = table[NK_COLOR_TEXT];
	//select->text_pressed_active = table[NK_COLOR_TEXT];
	//select->padding         = nk_vec2(2.0f,2.0f);
	//select->image_padding   = nk_vec2(2.0f,2.0f);
	//select->touch_padding   = nk_vec2(0,0);
	//select->userdata        = nk_handle_ptr(0);
	//select->rounding        = 0.0f;
	//select->draw_begin      = 0;
	//select->draw_end        = 0;

	char buffer[64];

	// Assumes all the style items are colors not images
	nk_layout_row_dynamic(ctx, 30, 2);
	nk_label(ctx, "Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Pressed:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.pressed.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.pressed.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.pressed.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Normal Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.normal_active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.normal_active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.normal_active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.hover_active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.hover_active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.hover_active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Pressed Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.pressed_active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.pressed_active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.pressed_active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.text_normal, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.text_normal), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.text_normal = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.text_hover, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.text_hover), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.text_hover = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Pressed:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.text_pressed, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.text_pressed), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.text_pressed = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Normal Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.text_normal_active, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.text_normal_active), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.text_normal_active = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Hover Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.text_hover_active, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.text_hover_active), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.text_hover_active = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Text Pressed Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, select.text_pressed_active, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(select.text_pressed_active), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		select.text_pressed_active = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", select.padding.x, select.padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &select.padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &select.padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Image Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", select.image_padding.x, select.image_padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &select.image_padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &select.image_padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Touch Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", select.touch_padding.x, select.touch_padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &select.touch_padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &select.touch_padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_property_float(ctx, "#Rounding:", -100.0f, &select.rounding, 100.0f, 1,0.5f);


	*out_style = select;
}

static void
style_slider(struct nk_context* ctx, struct nk_style_slider* out_style)
{
	struct nk_style_slider slider = *out_style;
	//slider = &style->slider;
	//nk_zero_struct(*slider);
	//slider->normal          = nk_style_item_hide();
	//slider->hover           = nk_style_item_hide();
	//slider->active          = nk_style_item_hide();
	//slider->bar_normal      = table[NK_COLOR_SLIDER];
	//slider->bar_hover       = table[NK_COLOR_SLIDER];
	//slider->bar_active      = table[NK_COLOR_SLIDER];
	//slider->bar_filled      = table[NK_COLOR_SLIDER_CURSOR];
	//slider->cursor_normal   = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR]);
	//slider->cursor_hover    = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_HOVER]);
	//slider->cursor_active   = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_ACTIVE]);
	//slider->inc_symbol      = NK_SYMBOL_TRIANGLE_RIGHT;
	//slider->dec_symbol      = NK_SYMBOL_TRIANGLE_LEFT;
	//slider->cursor_size     = nk_vec2(16,16);
	//slider->padding         = nk_vec2(2,2);
	//slider->spacing         = nk_vec2(2,2);
	//slider->userdata        = nk_handle_ptr(0);
	//slider->show_buttons    = nk_false;
	//slider->bar_height      = 8;
	//slider->rounding        = 0;
	//slider->draw_begin      = 0;
	//slider->draw_end        = 0;
	
	char buffer[64];

	// Assumes all the style items are colors not images
	nk_layout_row_dynamic(ctx, 30, 2);

	nk_label(ctx, "Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.normal.data.color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);
		colorf.a = nk_propertyf(ctx, "#A:", 0, colorf.a, 1.0f, 0.01f,0.005f);

		slider.normal.data.color = nk_rgba_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.hover.data.color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);
		colorf.a = nk_propertyf(ctx, "#A:", 0, colorf.a, 1.0f, 0.01f,0.005f);

		slider.hover.data.color = nk_rgba_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.active.data.color), NK_RGBA);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);
		colorf.a = nk_propertyf(ctx, "#A:", 0, colorf.a, 1.0f, 0.01f,0.005f);

		slider.active.data.color = nk_rgba_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Bar Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.bar_normal, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.bar_normal), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.bar_normal = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Bar Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.bar_hover, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.bar_hover), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.bar_hover = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Bar Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.bar_active, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.bar_active), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.bar_active = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Bar Filled:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.bar_filled, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.bar_filled), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.bar_filled = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.cursor_normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.cursor_normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.cursor_normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.cursor_hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.cursor_hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.cursor_hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, slider.cursor_active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(slider.cursor_active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		slider.cursor_active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}


	nk_label(ctx, "Cursor Size:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", slider.cursor_size.x, slider.cursor_size.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &slider.cursor_size.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &slider.cursor_size.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", slider.padding.x, slider.padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &slider.padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &slider.padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_label(ctx, "Spacing:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", slider.spacing.x, slider.spacing.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &slider.spacing.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &slider.spacing.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_property_float(ctx, "#Bar Height:", -100.0f, &slider.bar_height, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Rounding:", -100.0f, &slider.rounding, 100.0f, 1,0.5f);

	const char* symbols[NK_SYMBOL_MAX] =
{
    "NONE",
    "X",
    "UNDERSCORE",
    "CIRCLE_SOLID",
    "CIRCLE_OUTLINE",
    "RECT_SOLID",
    "RECT_OUTLINE",
    "TRIANGLE_UP",
    "TRIANGLE_DOWN",
    "TRIANGLE_LEFT",
    "TRIANGLE_RIGHT",
    "PLUS",
    "MINUS"
};

	nk_layout_row_dynamic(ctx, 30, 1);
	nk_checkbox_label(ctx, "Show Buttons", &slider.show_buttons);

	if (slider.show_buttons) {
		nk_layout_row_dynamic(ctx, 30, 2);
		nk_label(ctx, "Inc Symbol:", NK_TEXT_LEFT);
		slider.inc_symbol = nk_combo(ctx, symbols, NK_SYMBOL_MAX, slider.inc_symbol, 25, nk_vec2(200,200));
		nk_label(ctx, "Dec Symbol:", NK_TEXT_LEFT);
		slider.dec_symbol = nk_combo(ctx, symbols, NK_SYMBOL_MAX, slider.dec_symbol, 25, nk_vec2(200,200));

		// necessary or do tree's always take the whole width?
		//nk_layout_row_dynamic(ctx, 30, 1);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Slider Buttons", NK_MINIMIZED)) {
			struct nk_style_button* dups[1] = { &ctx->style.slider.dec_button };
			style_button(ctx, &ctx->style.slider.inc_button, dups, 1);
			nk_tree_pop(ctx);
		}

	}

	*out_style = slider;
}

static void
style_progress(struct nk_context* ctx, struct nk_style_progress* out_style)
{
	struct nk_style_progress prog = *out_style;
	//prog = &style->progress;
	//nk_zero_struct(*prog);
	//prog->normal            = nk_style_item_color(table[NK_COLOR_SLIDER]);
	//prog->hover             = nk_style_item_color(table[NK_COLOR_SLIDER]);
	//prog->active            = nk_style_item_color(table[NK_COLOR_SLIDER]);
	//prog->cursor_normal     = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR]);
	//prog->cursor_hover      = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_HOVER]);
	//prog->cursor_active     = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_ACTIVE]);
	//prog->border_color      = nk_rgba(0,0,0,0);
	//prog->cursor_border_color = nk_rgba(0,0,0,0);
	//prog->userdata          = nk_handle_ptr(0);
	//prog->padding           = nk_vec2(4,4);
	//prog->rounding          = 0;
	//prog->border            = 0;
	//prog->cursor_rounding   = 0;
	//prog->cursor_border     = 0;
	//prog->draw_begin        = 0;
	//prog->draw_end          = 0;
	
	char buffer[64];

	// Assumes all the style items are colors not images
	nk_layout_row_dynamic(ctx, 30, 2);

	nk_label(ctx, "Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}


	nk_label(ctx, "Cursor Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.cursor_normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.cursor_normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.cursor_normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.cursor_hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.cursor_hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.cursor_hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.cursor_active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.cursor_active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.cursor_active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Border Color:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.border_color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.border_color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.border_color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Border Color:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, prog.cursor_border_color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(prog.cursor_border_color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		prog.cursor_border_color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", prog.padding.x, prog.padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &prog.padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &prog.padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_property_float(ctx, "#Rounding:", -100.0f, &prog.rounding, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Border:", -100.0f, &prog.border, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Cursor Rounding:", -100.0f, &prog.cursor_rounding, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Cursor Border:", -100.0f, &prog.cursor_border, 100.0f, 1,0.5f);


	*out_style = prog;
}

static void
style_scrollbars(struct nk_context* ctx, struct nk_style_scrollbar* out_style, struct nk_style_scrollbar** duplicate_styles, int n_dups)
{
	struct nk_style_scrollbar scroll = *out_style;
	//scroll = &style->scrollh;
	//nk_zero_struct(*scroll);
	//scroll->normal          = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
	//scroll->hover           = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
	//scroll->active          = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
	//scroll->cursor_normal   = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR]);
	//scroll->cursor_hover    = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR_HOVER]);
	//scroll->cursor_active   = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE]);
	//scroll->dec_symbol      = NK_SYMBOL_CIRCLE_SOLID;
	//scroll->inc_symbol      = NK_SYMBOL_CIRCLE_SOLID;
	//scroll->userdata        = nk_handle_ptr(0);
	//scroll->border_color    = table[NK_COLOR_SCROLLBAR];
	//scroll->cursor_border_color = table[NK_COLOR_SCROLLBAR];
	//scroll->padding         = nk_vec2(0,0);
	//scroll->show_buttons    = nk_false;
	//scroll->border          = 0;
	//scroll->rounding        = 0;
	//scroll->border_cursor   = 0;
	//scroll->rounding_cursor = 0;
	//scroll->draw_begin      = 0;
	//scroll->draw_end        = 0;
	//style->scrollv = style->scrollh;
	
	char buffer[64];

	// Assumes all the style items are colors not images
	nk_layout_row_dynamic(ctx, 30, 2);

	nk_label(ctx, "Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}


	nk_label(ctx, "Cursor Normal:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.cursor_normal.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.cursor_normal.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.cursor_normal.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Hover:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.cursor_hover.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.cursor_hover.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.cursor_hover.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Active:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.cursor_active.data.color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.cursor_active.data.color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.cursor_active.data.color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Border Color:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.border_color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.border_color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.border_color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Cursor Border Color:", NK_TEXT_LEFT);
	if (nk_combo_begin_color(ctx, scroll.cursor_border_color, nk_vec2(nk_widget_width(ctx), 400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(scroll.cursor_border_color), NK_RGB);
		nk_layout_row_dynamic(ctx, 25, 1);
		colorf.r = nk_propertyf(ctx, "#R:", 0, colorf.r, 1.0f, 0.01f,0.005f);
		colorf.g = nk_propertyf(ctx, "#G:", 0, colorf.g, 1.0f, 0.01f,0.005f);
		colorf.b = nk_propertyf(ctx, "#B:", 0, colorf.b, 1.0f, 0.01f,0.005f);

		scroll.cursor_border_color = nk_rgb_cf(colorf);

		nk_combo_end(ctx);
	}

	nk_label(ctx, "Padding:", NK_TEXT_LEFT);
	sprintf(buffer, "%.2f, %.2f", scroll.padding.x, scroll.padding.y);
	if (nk_combo_begin_label(ctx, buffer, nk_vec2(200,200))) {
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, "#X:", -100.0f, &scroll.padding.x, 100.0f, 1,0.5f);
		nk_property_float(ctx, "#Y:", -100.0f, &scroll.padding.y, 100.0f, 1,0.5f);
		nk_combo_end(ctx);
	}

	nk_property_float(ctx, "#Border:", -100.0f, &scroll.border, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Rounding:", -100.0f, &scroll.rounding, 100.0f, 1,0.5f);

	// TODO naming inconsistency with style_scrollress?
	nk_property_float(ctx, "#Cursor Border:", -100.0f, &scroll.border_cursor, 100.0f, 1,0.5f);
	nk_property_float(ctx, "#Cursor Rounding:", -100.0f, &scroll.rounding_cursor, 100.0f, 1,0.5f);


	const char* symbols[NK_SYMBOL_MAX] =
{
    "NONE",
    "X",
    "UNDERSCORE",
    "CIRCLE_SOLID",
    "CIRCLE_OUTLINE",
    "RECT_SOLID",
    "RECT_OUTLINE",
    "TRIANGLE_UP",
    "TRIANGLE_DOWN",
    "TRIANGLE_LEFT",
    "TRIANGLE_RIGHT",
    "PLUS",
    "MINUS"
};

	// TODO what is wrong with scrollbar buttons?  Also look into controlling the total width (and height) of scrollbars
	nk_layout_row_dynamic(ctx, 30, 1);
	nk_checkbox_label(ctx, "Show Buttons", &scroll.show_buttons);

	if (scroll.show_buttons) {
		nk_layout_row_dynamic(ctx, 30, 2);
		nk_label(ctx, "Inc Symbol:", NK_TEXT_LEFT);
		scroll.inc_symbol = nk_combo(ctx, symbols, NK_SYMBOL_MAX, scroll.inc_symbol, 25, nk_vec2(200,200));
		nk_label(ctx, "Dec Symbol:", NK_TEXT_LEFT);
		scroll.dec_symbol = nk_combo(ctx, symbols, NK_SYMBOL_MAX, scroll.dec_symbol, 25, nk_vec2(200,200));

		//nk_layout_row_dynamic(ctx, 30, 1);
		if (nk_tree_push(ctx, NK_TREE_TAB, "Scrollbar Buttons", NK_MINIMIZED)) {
			// TODO best way to handle correctly with duplicate styles
			struct nk_style_button* dups[3] = { &ctx->style.scrollh.dec_button,
			                                    &ctx->style.scrollv.inc_button,
			                                    &ctx->style.scrollv.dec_button };
			style_button(ctx, &ctx->style.scrollh.inc_button, dups, 3);
			nk_tree_pop(ctx);
		}
	}


	//style->scrollv = style->scrollh;
	
	*out_style = scroll;
	if (duplicate_styles) {
		for (int i=0; i<n_dups; ++i) {
			*duplicate_styles[i] = scroll;
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
	struct nk_style_edit *edit;
	struct nk_style_property *property;
	struct nk_style_combo *combo;
	struct nk_style_chart *chart;
	struct nk_style_tab *tab;
	struct nk_style_window *win;

	// TODO better way?
	static int initialized = nk_false;
	static struct nk_color color_table[NK_COLOR_COUNT];

	if (!initialized) {
		memcpy(color_table, nk_default_color_style, sizeof(color_table));
		initialized = nk_true;
	}


	if (nk_begin(ctx, "Configurator", nk_rect(10, 10, 400, 600), window_flags))
	{
		if (nk_tree_push(ctx, NK_TREE_TAB, "Colors", NK_MINIMIZED)) {
			style_colors(ctx, color_table);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Text", NK_MINIMIZED)) {
			//text = &style->text;
			//text->color = table[NK_COLOR_TEXT];
			//text->padding = nk_vec2(0,0);

			char buffer[64];

			nk_layout_row_dynamic(ctx, 30, 2);
			nk_label(ctx, "Color:", NK_TEXT_LEFT);
			if (nk_combo_begin_color(ctx, text.color, nk_vec2(nk_widget_width(ctx), 400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				struct nk_colorf colorf = nk_color_picker(ctx, nk_color_cf(text.color), NK_RGB);
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
		if (nk_tree_push(ctx, NK_TREE_TAB, "Node Min/Max Buttons", NK_MINIMIZED)) {
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

		if (nk_tree_push(ctx, NK_TREE_TAB, "Checkbox", NK_MINIMIZED)) {
			style_toggle(ctx, &style->checkbox);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Option", NK_MINIMIZED)) {
			style_toggle(ctx, &style->option);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Selectable", NK_MINIMIZED)) {
			style_selectable(ctx, &style->selectable);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Slider", NK_MINIMIZED)) {
			style_slider(ctx, &style->slider);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Progress", NK_MINIMIZED)) {
			style_progress(ctx, &style->progress);
			nk_tree_pop(ctx);
		}


		if (nk_tree_push(ctx, NK_TREE_TAB, "Scrollbars", NK_MINIMIZED)) {
			struct nk_style_scrollbar* dups[1] = { &style->scrollv };
			style_scrollbars(ctx, &style->scrollh, dups, 1);
			nk_tree_pop(ctx);
		}



		if (nk_button_label(ctx, "Reset all styles to defaults")) {
			nk_style_default(ctx);
		}

	}

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

