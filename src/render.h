#pragma once

// Any sample needs to implement these functions.

// Main application cycle
void render_init();
void render_resized(int x, int y);
void render_update();
void render_redraw();
void render_destroy();

// Input information
void render_key_pressed(char key);
void render_key_down(char key);
void render_mouse_move(const glm::vec2& pos);
void render_mouse_down(const glm::vec2& pos, bool right = false);
void render_mouse_up(const glm::vec2& pos);