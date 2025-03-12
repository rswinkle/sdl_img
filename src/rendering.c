


void render_thumbs(void)
{
	SDL_SetRenderDrawColor(g->ren, g->bg.r, g->bg.g, g->bg.b, g->bg.a);
	SDL_RenderSetClipRect(g->ren, NULL);
	SDL_RenderClear(g->ren);

	struct nk_color th = g->thumb_highlight;
	int start = g->thumb_start_row * g->thumb_cols;
	int end = start + g->thumb_cols*g->thumb_rows;
	float w = g->scr_rect.w/(float)g->thumb_cols;
	float h = (g->scr_h - g->gui_bar_ht)/(float)g->thumb_rows;

	SDL_FRect r = { g->scr_rect.x, g->scr_rect.y, w, h };
	float w_over_thm_sz = w/(float)THUMBSIZE;
	float h_over_thm_sz = h/(float)THUMBSIZE;

	int cur_row = g->thumb_start_row - 1;
	for (int i = start; i < end && i<g->files.size; ++i) {
		cur_row += !(i % g->thumb_cols);
		// We create tex's in sequence and exit if any fail and
		// erase them when the source image is deleted so
		// we can break rather than continue here
		//
		// EDIT: with bad paths in list we could fail to create
		// a thumb but we also have never removed bad paths/non-images
		// so we have to continue
		if (!g->thumbs.a[i].tex) {
			//break;
			continue;
		}

		// to fill screen use these rather than following 4 lines
		//r.x = ((i-start) % g->thumb_cols) * w;
		//r.y = ((i-start) / g->thumb_cols) * h;

		// scales and centers thumbs appropriately
		r.w = g->thumbs.a[i].w * w_over_thm_sz;
		r.h = g->thumbs.a[i].h * h_over_thm_sz;
		r.x = (((i-start) % g->thumb_cols) * w) + (w-r.w)*0.5f;
		r.y = (((i-start) / g->thumb_cols) * h) + (h-r.h)*0.5f;

		SDL_RenderCopyF(g->ren, g->thumbs.a[i].tex, NULL, &r);
		if (g->state & THUMB_DFLT) {
			if (i == g->thumb_sel) {
				SDL_SetRenderDrawColor(g->ren, th.r, th.g, th.b, 255);
				// have selection box take up whole screen space, easier to see
				r.x = ((i-start) % g->thumb_cols) * w;
				r.y = ((i-start) / g->thumb_cols) * h;
				r.w = w;
				r.h = h;
				SDL_RenderDrawRectF(g->ren, &r);
			}
		} else if (g->state & THUMB_VISUAL) {
			if (!g->is_thumb_visual_line) {
				if ((i >= g->thumb_sel && i <= g->thumb_sel_end) ||
					(i <= g->thumb_sel && i >= g->thumb_sel_end)) {
					// TODO why doesn't setting this in setup work?  Where else is it changed?
					SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND);

					SDL_SetRenderDrawColor(g->ren, th.r, th.g, th.b, g->thumb_opacity);
					// have selection box take up whole screen space, easier to see
					r.x = ((i-start) % g->thumb_cols) * w;
					r.y = ((i-start) / g->thumb_cols) * h;
					r.w = w;
					r.h = h;
					SDL_RenderFillRectF(g->ren, &r);
				}
			} else {
				// Seems a bit stupid to do individual highlighting when we know
				// it's entire rows but meh.  TODO
				int r1 = g->thumb_sel / g->thumb_cols;
				int r2 = g->thumb_sel_end / g->thumb_cols;
				if (r2 < r1) {
					int tmp = r1;
					r1 = r2;
					r2 = tmp;
				}
				if (cur_row >= r1 && cur_row <= r2) {
					SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND);

					SDL_SetRenderDrawColor(g->ren, 0, 255, 0, g->thumb_opacity);
					// have selection box take up whole screen space, easier to see
					r.x = ((i-start) % g->thumb_cols) * w;
					r.y = ((i-start) / g->thumb_cols) * h;
					r.w = w;
					r.h = h;
					SDL_RenderFillRectF(g->ren, &r);
				}
			}
		} else if (g->state & SEARCH_RESULTS) {

			// TODO optimize since results are in order
			for (int k = 0; k<g->search_results.size; ++k) {
				if (g->search_results.a[k] == i) {
					SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND);

					SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 100);
					// have selection box take up whole screen space, easier to see
					r.x = ((i-start) % g->thumb_cols) * w;
					r.y = ((i-start) / g->thumb_cols) * h;
					r.w = w;
					r.h = h;
					SDL_RenderFillRectF(g->ren, &r);
					break;
				}
			}
			if (g->thumb_sel == i) {
				SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 255);
				SDL_RenderDrawRectF(g->ren, &r);
			}

		}
	}
}

int render_normal(int ticks)
{
	int is_a_gif = 0;

	// normal mode
	for (int i=0; i<g->n_imgs; ++i) {
		if (g->img[i].frames > 1) {
			int frame = g->img[i].frame_i;
			if (!g->img[i].paused && (!g->progress_hovered || (g->n_imgs > 1 && g->img_focus != &g->img[i]))) {
				// TODO if I allow a higher MAX_GIF_FPS than screen FPS (especially if using ACCERATED renderer and VSYNC)
				// should I show every frame or possibly skip frames if enough time has passed for 2+ frames?
				if (ticks - g->img[i].frame_timer >= g->img[i].delays[frame]) {
					g->img[i].frame_i = (frame + 1) % g->img[i].frames;
					if (g->img[i].frame_i == 0)
						g->img[i].looped = 1;
					g->img[i].frame_timer = ticks; // should be set after present ...
					g->status = REDRAW;
				}
			}
			is_a_gif = 1;
		}
	}

	// For ACCELERATED rendering with VSYNC it seems counter-intuitive but
	// the computer seems to work harder (fan spins up) when I only RenderPresent
	// when needed even if I also avoid all this extra work.
	//
	// And if I mix the two approaches, RenderPresent every frame but do
	// this work only when needed, there's weird ghosting/artifacts unless I
	// render an extra frame to handle hardware double buffering... so that's
	// what I've ended up doing because an extra frame won't hurt software rendering
	// TODO rename g->status g->render_status or frame_status or something
	if (g->show_gui || g->status)
	{
		// gui drawing changes draw color so have to reset to background every time
		SDL_SetRenderDrawColor(g->ren, g->bg.r, g->bg.g, g->bg.b, g->bg.a);
		SDL_RenderSetClipRect(g->ren, NULL);
		SDL_RenderClear(g->ren);
		for (int i=0; i<g->n_imgs; ++i) {
			SDL_RenderSetClipRect(g->ren, &g->img[i].scr_rect);
			SDL_RenderCopy(g->ren, g->img[i].tex[g->img[i].frame_i], NULL, &g->img[i].disp_rect);
		}
		SDL_RenderSetClipRect(g->ren, NULL); // reset for gui drawing

		// go from REDRAW to REDRAW2 to NOCHANGE ie (0)
		g->status = (g->status + 1) % NUM_STATUSES;
	}
	
	return is_a_gif;
}

