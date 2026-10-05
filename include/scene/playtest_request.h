#pragma once
#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

/// 编辑器「试玩」的交接通道：把**当前编辑中**（未保存）的关卡从编辑器交给关卡场景。
///
/// 为什么需要它：`GameScene` 只认磁盘上的关卡序号（`levelIndex_`），而试玩的语义
/// 恰恰是"玩你现在改到一半的那份"。既有的交接范式是"共享服务持有待处理数据"
/// （`SaveManager::setPendingSave`），这里照抄那个形状 —— 而不是给场景加一堆
/// 只有试玩才用得上的字段。
///
/// ⚠️ **取走是一次性的**：`take()` 之后请求就没了。否则试玩结束回到编辑器、
///  以后再从选关页进普通关卡，会莫名其妙又玩到那份旧草稿 —— 而且没有任何提示，
///  看起来就像"关卡加载错了"。
class PlaytestRequest {
public:
    struct Request {
        std::vector<std::string> lines; ///< 关卡的 ASCII 文本（一行一条）
        std::string sourceName;         ///< 来源文件名，只用于显示
    };

    /// 编辑器按 F5 时调用
    void request(std::vector<std::string> lines, std::string sourceName) {
        pending_ = Request{std::move(lines), std::move(sourceName)};
    }

    /// 取走（取一次就清空）。没有待处理请求时返回 nullopt。
    std::optional<Request> take() {
        auto out = std::move(pending_);
        pending_.reset();
        return out;
    }

    bool has() const { return pending_.has_value(); }
    void clear() { pending_.reset(); }

    /// 编辑器内存里的关卡行 → 关卡文本（逐行拼接、每行末尾补 '\n'）。
    ///
    /// ⚠️ **编辑器与测试必须调这一个函数**。测试里手抄一份转换的话，
    /// 它验证的只是"抄得对不对"——而真正会出错的是生产代码那一份。
    /// （第一版测试就是这么写的，做完才发现它根本管不到
    /// `EditorScene::startPlaytest` 里的拼接。）
    static std::string toLevelText(const std::vector<std::string>& lines) {
        std::string text;
        for (const auto& line : lines) {
            text += line;
            text += '\n';
        }
        return text;
    }

    /// 把读到的行补成**等宽矩形**（编辑器按最长行算宽度）。
    ///
    /// 单独抽出来同样是为了让测试能验真实实现：磁盘上的关卡文件可以长短不一，
    /// 而 `Level` 的解析是按矩形网格做的。
    static void normalizeLines(std::vector<std::string>& lines) {
        std::size_t w = 0;
        for (const auto& l : lines)
            w = std::max(w, l.size());
        for (auto& l : lines)
            l.resize(w, ' ');
    }

    /// 这份草稿能不能试玩 —— 至少要有一个玩家出生点 'P'。
    ///
    /// 没有 'P' 的话关卡**确实**能构造出来，但玩家会被摆在 (0,0)，
    /// 大概率正好卡在边界墙里：画面在动、按什么都没反应。与其让用户对着一个
    /// 动不了的画面发愣，不如在按 F5 的时候就说清楚。
    static bool hasPlayerSpawn(const std::vector<std::string>& lines) {
        for (const auto& line : lines)
            if (line.find('P') != std::string::npos)
                return true;
        return false;
    }

private:
    std::optional<Request> pending_;
};
