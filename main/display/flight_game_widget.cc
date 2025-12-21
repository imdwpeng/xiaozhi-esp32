#include "flight_game_widget.h"
#include "esp_log.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

static const char* TAG = "FlightGameWidget";

// 游戏配置
static const int AIRCRAFT_SPEED = 5;
static const int BULLET_SPEED = 8;
static const int ENEMY_SPEED_MIN = 2;
static const int ENEMY_SPEED_MAX = 4;
static const int CLOUD_SPEED = 1;
static const int MAX_BULLETS = 3;
static const int MAX_ENEMIES = 5;
static const int MAX_CLOUDS = 3;

FlightGameWidget::FlightGameWidget() {
    container_ = nullptr;
    score_label_ = nullptr;
    life_label_ = nullptr;
    info_label_ = nullptr;
    game_area_ = nullptr;
    aircraft_ = nullptr;
    game_state_ = GAME_STATE_INIT;
    score_ = 0;
    lives_ = 3;
    game_width_ = 240;
    game_height_ = 280;
    left_pressed_ = false;
    right_pressed_ = false;
    game_timer_ = nullptr;
    bullet_timer_ = nullptr;
    enemy_timer_ = nullptr;
    cloud_timer_ = nullptr;
}

FlightGameWidget::~FlightGameWidget() {
    Destroy();
}

void FlightGameWidget::Create(lv_obj_t* parent) {
    ESP_LOGI(TAG, "Creating flight game widget");
    
    // 创建容器
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, game_width_, game_height_);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x001133), LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(container_, lv_color_hex(0x4488ff), LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 10, LV_PART_MAIN);
    lv_obj_center(container_);
    
    CreateUI();
    InitializeGame();
    
    ESP_LOGI(TAG, "Flight game widget created successfully");
}

void FlightGameWidget::Destroy() {
    // 停止所有定时器
    if (game_timer_) {
        lv_timer_del(game_timer_);
        game_timer_ = nullptr;
    }
    if (bullet_timer_) {
        lv_timer_del(bullet_timer_);
        bullet_timer_ = nullptr;
    }
    if (enemy_timer_) {
        lv_timer_del(enemy_timer_);
        enemy_timer_ = nullptr;
    }
    if (cloud_timer_) {
        lv_timer_del(cloud_timer_);
        cloud_timer_ = nullptr;
    }
    
    // 清理游戏对象
    game_objects_.clear();
    
    // 销毁UI
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        score_label_ = nullptr;
        life_label_ = nullptr;
        info_label_ = nullptr;
        game_area_ = nullptr;
        aircraft_ = nullptr;
    }
}

void FlightGameWidget::SetPosition(int x, int y) {
    if (container_) {
        lv_obj_set_pos(container_, x, y);
    }
}

void FlightGameWidget::SetSize(int width, int height) {
    game_width_ = width;
    game_height_ = height;
    if (container_) {
        lv_obj_set_size(container_, width, height);
    }
}

void FlightGameWidget::CreateUI() {
    // 创建分数标签
    score_label_ = lv_label_create(container_);
    lv_label_set_text(score_label_, "分数: 0");
    lv_obj_set_style_text_font(score_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(score_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(score_label_, LV_ALIGN_TOP_LEFT, 10, 5);
    
    // 创建生命值标签
    life_label_ = lv_label_create(container_);
    lv_label_set_text(life_label_, "生命: ❤❤❤");
    lv_obj_set_style_text_font(life_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(life_label_, lv_color_hex(0xff4444), LV_PART_MAIN);
    lv_obj_align(life_label_, LV_ALIGN_TOP_RIGHT, -10, 5);
    
    // 创建信息标签
    info_label_ = lv_label_create(container_);
    lv_label_set_text(info_label_, "按中间键开始游戏");
    lv_obj_set_style_text_font(info_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(info_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(info_label_, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // 创建游戏区域
    game_area_ = lv_obj_create(container_);
    lv_obj_set_size(game_area_, game_width_ - 20, game_height_ - 60);
    lv_obj_set_pos(game_area_, 10, 30);
    lv_obj_set_style_bg_color(game_area_, lv_color_hex(0x000044), LV_PART_MAIN);
    lv_obj_set_style_border_width(game_area_, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(game_area_, 5, LV_PART_MAIN);
}

void FlightGameWidget::InitializeGame() {
    ESP_LOGI(TAG, "Initializing game");
    
    // 初始化随机种子
    srand(esp_timer_get_time() / 1000);
    
    // 重置游戏状态
    score_ = 0;
    lives_ = 3;
    game_state_ = GAME_STATE_INIT;
    left_pressed_ = false;
    right_pressed_ = false;
    
    // 清理现有对象
    game_objects_.clear();
    aircraft_ = nullptr;
    
    // 创建飞机
    CreateAircraft();
    
    // 更新UI
    UpdateScore();
    UpdateLives();
    ShowInitScreen();
    
    // 设置按键模式为正常模式
    SetButtonMode(0);
}

void FlightGameWidget::CreateAircraft() {
    aircraft_ = CreateGameObject(GAME_OBJ_AIRCRAFT, game_width_ / 2 - 15, game_height_ - 80);
    if (aircraft_) {
        aircraft_->width = 30;
        aircraft_->height = 30;
        aircraft_->speed = AIRCRAFT_SPEED;
        
        lv_obj_set_style_bg_color(aircraft_->obj, lv_color_hex(0x00ff00), LV_PART_MAIN);
        lv_obj_set_style_text_font(aircraft_->obj, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(aircraft_->obj, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_label_set_text(static_cast<lv_obj_t*>(aircraft_->obj), "✈️");
    }
}

game_object_t* FlightGameWidget::CreateGameObject(game_object_type_t type, int x, int y) {
    auto obj = std::make_unique<game_object_t>();
    obj->type = type;
    obj->x = x;
    obj->y = y;
    obj->active = true;
    obj->speed = 0;
    
    // 创建LVGL对象
    obj->obj = lv_label_create(game_area_);
    lv_obj_set_pos(obj->obj, x, y);
    
    game_object_t* ptr = obj.get();
    game_objects_.push_back(std::move(obj));
    return ptr;
}

void FlightGameWidget::StartGame() {
    if (game_state_ != GAME_STATE_INIT) return;
    
    ESP_LOGI(TAG, "Starting game");
    game_state_ = GAME_STATE_PLAYING;
    
    // 切换到游戏模式
    SetButtonMode(1);
    
    // 清理现有对象
    game_objects_.clear();
    CreateAircraft();
    
    // 启动定时器
    game_timer_ = lv_timer_create([](lv_timer_t* timer) {
        FlightGameWidget* game = static_cast<FlightGameWidget*>(lv_timer_get_user_data(timer));
        game->UpdateGame();
    }, 20, this); // 50Hz
    
    bullet_timer_ = lv_timer_create([](lv_timer_t* timer) {
        FlightGameWidget* game = static_cast<FlightGameWidget*>(lv_timer_get_user_data(timer));
        game->CreateBullet();
    }, 500, this); // 每500ms发射子弹
    
    enemy_timer_ = lv_timer_create([](lv_timer_t* timer) {
        FlightGameWidget* game = static_cast<FlightGameWidget*>(lv_timer_get_user_data(timer));
        game->CreateEnemy();
    }, 1500, this); // 每1.5秒生成敌人
    
    cloud_timer_ = lv_timer_create([](lv_timer_t* timer) {
        FlightGameWidget* game = static_cast<FlightGameWidget*>(lv_timer_get_user_data(timer));
        game->CreateCloud();
    }, 2000, this); // 每2秒生成云朵
    
    UpdateInfo();
    SetButtonMode(1); // 切换到游戏模式
}

void FlightGameWidget::PauseGame() {
    if (game_state_ != GAME_STATE_PLAYING) return;
    
    ESP_LOGI(TAG, "Pausing game");
    game_state_ = GAME_STATE_PAUSED;
    
    // 切换到暂停模式
    SetButtonMode(2);
    
    // 停止游戏定时器
    if (game_timer_) {
        lv_timer_pause(game_timer_);
    }
    if (bullet_timer_) {
        lv_timer_pause(bullet_timer_);
    }
    if (enemy_timer_) {
        lv_timer_pause(enemy_timer_);
    }
    if (cloud_timer_) {
        lv_timer_pause(cloud_timer_);
    }
    
    ShowPauseScreen();
    SetButtonMode(2); // 切换到暂停模式
}

void FlightGameWidget::ResumeGame() {
    if (game_state_ != GAME_STATE_PAUSED) return;
    
    ESP_LOGI(TAG, "Resuming game");
    game_state_ = GAME_STATE_PLAYING;
    
    // 切换到游戏模式
    SetButtonMode(1);
    
    // 恢复定时器
    if (game_timer_) {
        lv_timer_resume(game_timer_);
    }
    if (bullet_timer_) {
        lv_timer_resume(bullet_timer_);
    }
    if (enemy_timer_) {
        lv_timer_resume(enemy_timer_);
    }
    if (cloud_timer_) {
        lv_timer_resume(cloud_timer_);
    }
    
    UpdateInfo();
    SetButtonMode(1); // 切换到游戏模式
}

void FlightGameWidget::ExitGame() {
    ESP_LOGI(TAG, "Exiting game");
    
    // 切换到正常模式
    SetButtonMode(0);
    
    // 停止所有定时器
    if (game_timer_) {
        lv_timer_del(game_timer_);
        game_timer_ = nullptr;
    }
    if (bullet_timer_) {
        lv_timer_del(bullet_timer_);
        bullet_timer_ = nullptr;
    }
    if (enemy_timer_) {
        lv_timer_del(enemy_timer_);
        enemy_timer_ = nullptr;
    }
    if (cloud_timer_) {
        lv_timer_del(cloud_timer_);
        cloud_timer_ = nullptr;
    }
    
    // 重置游戏
    InitializeGame();
}

void FlightGameWidget::SaveAndExit() {
    ESP_LOGI(TAG, "Saving and exiting game");
    // 这里可以实现保存游戏状态的逻辑
    
    // 切换到正常模式
    SetButtonMode(0);
    
    ExitGame();
}

void FlightGameWidget::HandleButtonPress(int button_id, bool pressed) {
    if (game_state_ == GAME_STATE_INIT) {
        if (button_id == 2 && pressed) { // 中间键
            StartGame();
        }
    } else if (game_state_ == GAME_STATE_PLAYING) {
        if (button_id == 0) { // 左键
            left_pressed_ = pressed;
        } else if (button_id == 1) { // 右键
            right_pressed_ = pressed;
        } else if (button_id == 2 && pressed) { // 中间键长按
            PauseGame();
        }
    } else if (game_state_ == GAME_STATE_PAUSED) {
        if (button_id == 0 && pressed) { // 左键
            ExitGame();
        } else if (button_id == 1 && pressed) { // 右键
            SaveAndExit();
        } else if (button_id == 2 && pressed) { // 中间键
            ResumeGame();
        }
    }
}

void FlightGameWidget::UpdateGame() {
    if (game_state_ != GAME_STATE_PLAYING) return;
    
    UpdateAircraft();
    UpdateBullets();
    UpdateEnemies();
    UpdateClouds();
    CheckCollisions();
    CleanupInactiveObjects();
}

void FlightGameWidget::UpdateAircraft() {
    if (!aircraft_ || !aircraft_->active) return;
    
    int target_x = aircraft_->x;
    if (left_pressed_) {
        target_x -= aircraft_->speed;
    }
    if (right_pressed_) {
        target_x += aircraft_->speed;
    }
    
    // 限制飞机在游戏区域内
    int game_area_width = game_width_ - 40;
    if (target_x < 0) target_x = 0;
    if (target_x > game_area_width - aircraft_->width) {
        target_x = game_area_width - aircraft_->width;
    }
    
    aircraft_->x = target_x;
    lv_obj_set_pos(aircraft_->obj, aircraft_->x, aircraft_->y);
}

void FlightGameWidget::UpdateBullets() {
    for (auto& obj : game_objects_) {
        if (obj->type == GAME_OBJ_BULLET && obj->active) {
            obj->y -= BULLET_SPEED;
            if (obj->y < -20) {
                obj->active = false;
            } else {
                lv_obj_set_pos(obj->obj, obj->x, obj->y);
            }
        }
    }
}

void FlightGameWidget::UpdateEnemies() {
    for (auto& obj : game_objects_) {
        if (obj->type == GAME_OBJ_ENEMY && obj->active) {
            obj->y += obj->speed;
            if (obj->y > game_height_) {
                obj->active = false;
            } else {
                lv_obj_set_pos(obj->obj, obj->x, obj->y);
            }
        }
    }
}

void FlightGameWidget::UpdateClouds() {
    for (auto& obj : game_objects_) {
        if (obj->type == GAME_OBJ_CLOUD && obj->active) {
            obj->y += CLOUD_SPEED;
            if (obj->y > game_height_) {
                obj->active = false;
            } else {
                lv_obj_set_pos(obj->obj, obj->x, obj->y);
            }
        }
    }
}

void FlightGameWidget::CreateBullet() {
    if (game_state_ != GAME_STATE_PLAYING) return;
    
    // 限制子弹数量
    int bullet_count = 0;
    for (const auto& obj : game_objects_) {
        if (obj->type == GAME_OBJ_BULLET && obj->active) {
            bullet_count++;
        }
    }
    if (bullet_count >= MAX_BULLETS) return;
    
    if (!aircraft_ || !aircraft_->active) return;
    
    auto bullet = CreateGameObject(GAME_OBJ_BULLET, aircraft_->x + 12, aircraft_->y);
    if (bullet) {
        bullet->width = 6;
        bullet->height = 12;
        bullet->speed = BULLET_SPEED;
        
        lv_obj_set_style_text_font(bullet->obj, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(bullet->obj, lv_color_hex(0xffff00), LV_PART_MAIN);
        lv_label_set_text(static_cast<lv_obj_t*>(bullet->obj), "•");
    }
}

void FlightGameWidget::CreateEnemy() {
    if (game_state_ != GAME_STATE_PLAYING) return;
    
    // 限制敌人数量
    int enemy_count = 0;
    for (const auto& obj : game_objects_) {
        if (obj->type == GAME_OBJ_ENEMY && obj->active) {
            enemy_count++;
        }
    }
    if (enemy_count >= MAX_ENEMIES) return;
    
    int game_area_width = game_width_ - 40;
    int x = rand() % (game_area_width - 25);
    int speed = ENEMY_SPEED_MIN + (rand() % (ENEMY_SPEED_MAX - ENEMY_SPEED_MIN + 1));
    
    auto enemy = CreateGameObject(GAME_OBJ_ENEMY, x, -25);
    if (enemy) {
        enemy->width = 25;
        enemy->height = 25;
        enemy->speed = speed;
        
        lv_obj_set_style_text_font(enemy->obj, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(enemy->obj, lv_color_hex(0xff4444), LV_PART_MAIN);
        lv_label_set_text(static_cast<lv_obj_t*>(enemy->obj), "👾");
    }
}

void FlightGameWidget::CreateCloud() {
    if (game_state_ != GAME_STATE_PLAYING) return;
    
    // 限制云朵数量
    int cloud_count = 0;
    for (const auto& obj : game_objects_) {
        if (obj->type == GAME_OBJ_CLOUD && obj->active) {
            cloud_count++;
        }
    }
    if (cloud_count >= MAX_CLOUDS) return;
    
    int game_area_width = game_width_ - 40;
    int x = rand() % (game_area_width - 30);
    
    auto cloud = CreateGameObject(GAME_OBJ_CLOUD, x, -30);
    if (cloud) {
        cloud->width = 30;
        cloud->height = 20;
        cloud->speed = CLOUD_SPEED;
        
        lv_obj_set_style_text_font(cloud->obj, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(cloud->obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN);
        lv_label_set_text(static_cast<lv_obj_t*>(cloud->obj), "☁️");
    }
}

void FlightGameWidget::CheckCollisions() {
    if (!aircraft_ || !aircraft_->active) return;
    
    // 检查子弹与敌人的碰撞
    for (auto& bullet : game_objects_) {
        if (bullet->type == GAME_OBJ_BULLET && bullet->active) {
            for (auto& enemy : game_objects_) {
                if (enemy->type == GAME_OBJ_ENEMY && enemy->active) {
                    if (CheckCollision(bullet.get(), enemy.get())) {
                        bullet->active = false;
                        enemy->active = false;
                        score_ += 10;
                        UpdateScore();
                        CreateExplosion(enemy->x, enemy->y);
                    }
                }
            }
        }
    }
    
    // 检查飞机与敌人的碰撞
    for (auto& enemy : game_objects_) {
        if (enemy->type == GAME_OBJ_ENEMY && enemy->active) {
            if (CheckCollision(aircraft_, enemy.get())) {
                enemy->active = false;
                lives_--;
                UpdateLives();
                CreateExplosion(aircraft_->x, aircraft_->y);
                
                if (lives_ <= 0) {
                    // 游戏结束
                    ExitGame();
                    lv_label_set_text(info_label_, "游戏结束! 按中间键重新开始");
                }
            }
        }
    }
}

void FlightGameWidget::CreateExplosion(int x, int y) {
    auto explosion = CreateGameObject(GAME_OBJ_EXPLOSION, x, y);
    if (explosion) {
        lv_obj_set_style_text_font(explosion->obj, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(explosion->obj, lv_color_hex(0xff8800), LV_PART_MAIN);
        lv_label_set_text(static_cast<lv_obj_t*>(explosion->obj), "💥");
        
        // 1秒后删除爆炸效果
        lv_timer_create([](lv_timer_t* timer) {
            auto* obj = static_cast<game_object_t*>(lv_timer_get_user_data(timer));
            obj->active = false;
        }, 1000, explosion);
    }
}

bool FlightGameWidget::CheckCollision(game_object_t* obj1, game_object_t* obj2) {
    return (obj1->x < obj2->x + obj2->width &&
            obj1->x + obj1->width > obj2->x &&
            obj1->y < obj2->y + obj2->height &&
            obj1->y + obj1->height > obj2->y);
}

void FlightGameWidget::CleanupInactiveObjects() {
    auto it = game_objects_.begin();
    while (it != game_objects_.end()) {
        if (!(*it)->active) {
            if ((*it)->obj) {
                lv_obj_del((*it)->obj);
            }
            it = game_objects_.erase(it);
        } else {
            ++it;
        }
    }
}

void FlightGameWidget::UpdateScore() {
    if (score_label_) {
        static char score_text[32];
        snprintf(score_text, sizeof(score_text), "分数: %d", score_);
        lv_label_set_text(score_label_, score_text);
    }
}

void FlightGameWidget::UpdateLives() {
    if (life_label_) {
        static char life_text[32];
        char hearts[16] = "";
        for (int i = 0; i < lives_; i++) {
            strcat(hearts, "❤");
        }
        snprintf(life_text, sizeof(life_text), "生命: %s", hearts);
        lv_label_set_text(life_label_, life_text);
    }
}

void FlightGameWidget::UpdateInfo() {
    if (info_label_) {
        switch (game_state_) {
            case GAME_STATE_INIT:
                lv_label_set_text(info_label_, "按中间键开始游戏");
                break;
            case GAME_STATE_PLAYING:
                lv_label_set_text(info_label_, "长按中间键暂停游戏");
                break;
            case GAME_STATE_PAUSED:
                lv_label_set_text(info_label_, "按左键直接退出，按右键暂存退出");
                break;
        }
    }
}

void FlightGameWidget::ShowInitScreen() {
    UpdateInfo();
}

void FlightGameWidget::ShowPauseScreen() {
    UpdateInfo();
}

void FlightGameWidget::SetButtonMode(int mode) {
    // Touch mode handled by Touch Element Library
    // SimpleTouchManager is no longer needed
    
    // 也可以通过回调通知其他组件
    if (button_callback_) {
        button_callback_(mode, true);
    }
}