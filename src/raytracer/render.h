#pragma once


void render_resized();
void render_redraw();
void render_on_resize();
void render_init();
void render_update();
void render_redraw();
void render_key_pressed(char key);
void render_key_down(char key);
void render_mouse_move(const glm::vec2& pos);
void render_mouse_down(const glm::vec2& pos);
void render_mouse_up(const glm::vec2& pos);