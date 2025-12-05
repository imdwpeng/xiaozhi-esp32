#ifndef FLIGHT_GAME_WIDGET_H
#define FLIGHT_GAME_WIDGET_H

#include <lvgl.h>
#include <vector>
#include <memory>
#include <functional>
#include <esp_timer.h>

// 声明字体
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_14);

// 游戏状态
typedef enum {
    GAME_STATE_INIT = 0,     // 初始状态，等待开始
    GAME_STATE_PLAYING,      // 游戏进行中
    GAME_STATE_PAUSED        // 暂停/退出选择
} game_state_t;

// 游戏对象类型
typedef enum {
    GAME_OBJ_AIRCRAFT = 0,
    GAME_OBJ_BULLET,
    GAME_OBJ_ENEMY,
    GAME_OBJ_CLOUD,
    GAME_OBJ_EXPLOSION
} game_object_type_t;

// 游戏对象
typedef struct {
    lv_obj_t* obj;
    game_object_type_t type;
    int x, y;
    int width, height;
    bool active;
    int speed;
} game_object_t;

// 游戏按键回调类型
using GameButtonCallback = std::function<void(int button_id, bool pressed)>;

class FlightGameWidget {
public:
    FlightGameWidget();
    ~FlightGameWidget();
    
    // 创建游戏小部件
    void Create(lv_obj_t* parent);
    
    // 销毁游戏小部件
    void Destroy();
    
    // 设置位置和大小
    void SetPosition(int x, int y);
    void SetSize(int width, int height);
    
    // 设置按键回调
    void SetButtonCallback(GameButtonCallback callback) { button_callback_ = callback; }
    
    // 游戏控制
    void StartGame();
    void PauseGame();
    void ResumeGame();
    void ExitGame();
    void SaveAndExit();
    
    // 按键处理
    void HandleButtonPress(int button_id, bool pressed);
    
    // 获取游戏状态
    game_state_t GetGameState() const { return game_state_; }
    
private:
    // UI组件
    lv_obj_t* container_;
    lv_obj_t* score_label_;
    lv_obj_t* life_label_;
    lv_obj_t* info_label_;
    lv_obj_t* game_area_;
    
    // 游戏对象
    std::vector<std::unique_ptr<game_object_t>> game_objects_;
    game_object_t* aircraft_;
    
    // 游戏状态
    game_state_t game_state_;
    int score_;
    int lives_;
    int game_width_;
    int game_height_;
    bool left_pressed_;
    bool right_pressed_;
    
    // 定时器
    lv_timer_t* game_timer_;
    lv_timer_t* bullet_timer_;
    lv_timer_t* enemy_timer_;
    lv_timer_t* cloud_timer_;
    
    // 回调
    GameButtonCallback button_callback_;
    
    // 初始化函数
    void InitializeGame();
    void CreateUI();
    void CreateAircraft();
    
    // 游戏逻辑
    void UpdateGame();
    void UpdateAircraft();
    void UpdateBullets();
    void UpdateEnemies();
    void UpdateClouds();
    void CheckCollisions();
    
    // 对象管理
    game_object_t* CreateGameObject(game_object_type_t type, int x, int y);
    void DestroyGameObject(game_object_t* obj);
    void CleanupInactiveObjects();
    
    // 生成对象
    void CreateBullet();
    void CreateEnemy();
    void CreateCloud();
    void CreateExplosion(int x, int y);
    
    // UI更新
    void UpdateScore();
    void UpdateLives();
    void UpdateInfo();
    void ShowInitScreen();
    void ShowPauseScreen();
    
    // 辅助函数
    bool CheckCollision(game_object_t* obj1, game_object_t* obj2);
    void SetButtonMode(int mode); // 0=normal, 1=game, 2=pause
};

#endif // FLIGHT_GAME_WIDGET_H