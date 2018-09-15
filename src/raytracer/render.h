#pragma once


void render_resized(int x, int y);
void render_redraw();
void render_init();
void render_destroy();
void render_update();
void render_redraw();
void render_key_pressed(char key);
void render_key_down(char key);
void render_mouse_move(const glm::vec2& pos);
void render_mouse_down(const glm::vec2& pos);
void render_mouse_up(const glm::vec2& pos);