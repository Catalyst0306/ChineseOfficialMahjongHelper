﻿#include "CompetitionTableScene.h"
#include <array>
#include "../widget/AlertView.h"
#include "Competition.h"
#include "CompetitionRankCustomScene.h"
#include "EditBoxDelegateWrapper.hpp"

USING_NS_CC;

static const char *seatText[] = { "东", "南", "西", "北" };

bool CompetitionTableScene::initWithData(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound) {
    if (UNLIKELY(!BaseScene::initWithTitle(Common::format("%s第%" PRIzu "/%" PRIzu "轮",
        competitionData->name.c_str(), currentRound + 1, competitionData->round_count)))) {
        return false;
    }

    _competitionData = competitionData;
    _currentRound = currentRound;
    _competitionTables = &_competitionData->rounds[currentRound].tables;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 列宽
    _colWidth[0] = visibleSize.width * 0.08f;
    _colWidth[1] = visibleSize.width * 0.08f;
    _colWidth[2] = visibleSize.width * 0.08f;
    _colWidth[3] = visibleSize.width * 0.2f;
    _colWidth[4] = visibleSize.width * 0.14f;
    _colWidth[5] = visibleSize.width * 0.14f;
    _colWidth[6] = visibleSize.width * 0.14f;
    _colWidth[7] = visibleSize.width * 0.14f;

    // 中心位置
    Common::calculateColumnsCenterX(_colWidth, 8, _posX);

    // 表头
    static const char *titleTexts[] = { "桌号", "座次", "编号", "选手姓名", "顺位", "标准分", "比赛分" };
    for (int i = 0; i < 7; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(origin.x + _posX[i], visibleSize.height - 45.0f));
        Common::scaleLabelToFitWidth(label, _colWidth[i] - 4.0f);
    }

    const float tableHeight = visibleSize.height - 85.0f;

    // 表格
    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width, tableHeight));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 12.5f));
    tableView->reloadData();
    _tableView = tableView;
    this->addChild(tableView);

    // 表头的线
    DrawNode *drawNode = DrawNode::create();
    this->addChild(drawNode);
    drawNode->setPosition(Vec2(origin.x, visibleSize.height - 55.0f));
    drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(visibleSize.width, 0.0f), Color4F::BLACK);
    drawNode->drawLine(Vec2(0.0f, 20.0f), Vec2(visibleSize.width, 20.0f), Color4F::BLACK);
    for (int i = 0; i < 7; ++i) {
        const float posX = _posX[i] + _colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(posX, 0.0f), Vec2(posX, 20.0f), Color4F::BLACK);
    }

    // 当表格可拖动时，画下方一条线
    if (tableView->getInnerContainerSize().height > tableHeight) {
        float posY = -tableHeight;
        drawNode->drawLine(Vec2(0.0f, posY), Vec2(visibleSize.width, posY), Color4F::BLACK);
    }
    tableView->setOnEnterCallback(std::bind(&cw::TableView::reloadDataInplacement, tableView));

    // 排列座位按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("排列座位");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.25f, origin.y + 15.0f));
    button->addClickEventListener([this](Ref *) { showRankAlert(); });

    // 提交按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(50.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("提交");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.75f, origin.y + 15.0f));
    button->addClickEventListener([](Ref *) { cocos2d::Director::getInstance()->popScene(); });
    button->setEnabled(_competitionData->isRoundFinished(_currentRound));
    _submitButton = button;

    return true;
}

ssize_t CompetitionTableScene::numberOfCellsInTableView(cw::TableView *table) {
    return _competitionTables->size();
}

float CompetitionTableScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return 80.0f;
}

cw::TableViewCell *CompetitionTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, std::array<std::array<Label *, 5>, 4>, std::array<ui::Button *, 2>, std::array<LayerColor *, 2> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&tableLabel = std::get<0>(ext);
        std::array<std::array<Label *, 5>, 4> &labels = std::get<1>(ext);
        std::array<ui::Button *, 2> &buttons = std::get<2>(ext);
        std::array<LayerColor *, 2> &layerColors = std::get<3>(ext);

        Size visibleSize = Director::getInstance()->getVisibleSize();

        // 背景色
        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), visibleSize.width, 80.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 80.0f);
        cell->addChild(layerColors[1]);

        // 桌号
        tableLabel = Label::createWithSystemFont("", "Arail", 12);
        tableLabel->setColor(Color3B::BLACK);
        cell->addChild(tableLabel);
        tableLabel->setPosition(Vec2(_posX[0], 40.0f));

        // 座次
        for (int i = 0; i < 4; ++i) {
            const float posY = static_cast<float>(70 - i * 20);

            Label *label = Label::createWithSystemFont(seatText[i], "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[1], posY));
        }

        // 编号、选手姓名、顺位、标准分、比赛分，共5个Label
        Color3B textColor[] = { Color3B(0x60, 0x60, 0x60), Color3B::ORANGE, Color3B(254, 87, 110), Color3B(44, 121, 178), Color3B(101, 196, 59) };
        for (int i = 0; i < 4; ++i) {
            const float posY = static_cast<float>(70 - i * 20);

            for (int k = 0; k < 5; ++k) {
                Label *label = Label::createWithSystemFont("", "Arail", 12);
                label->setColor(textColor[k]);
                cell->addChild(label);
                label->setPosition(Vec2(_posX[2 + k], posY));
                labels[i][k] = label;
            }
        }

        // 输入按钮
        ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        cell->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(_colWidth[7] - 8, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("输入");
        button->setPosition(Vec2(_posX[7], 55.0f));
        button->addClickEventListener(std::bind(&CompetitionTableScene::onRecordButton, this, std::placeholders::_1));
        Common::scaleLabelToFitWidth(button->getTitleLabel(), _colWidth[6] - 10.0f);
        buttons[0] = button;

        // 清空按钮
        button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        cell->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(_colWidth[7] - 8, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("清空");
        button->setPosition(Vec2(_posX[7], 25.0f));
        button->addClickEventListener(std::bind(&CompetitionTableScene::onClearButton, this, std::placeholders::_1));
        Common::scaleLabelToFitWidth(button->getTitleLabel(), _colWidth[6] - 10.0f);
        buttons[1] = button;

        // 画线
        DrawNode *drawNode = DrawNode::create();
        cell->addChild(drawNode);
        drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(visibleSize.width, 0.0f), Color4F::BLACK);
        drawNode->drawLine(Vec2(0.0f, 80.0f), Vec2(visibleSize.width, 80.0f), Color4F::BLACK);
        const float posX = visibleSize.width - _colWidth[6];
        for (int i = 0; i < 3; ++i) {
            const float posY = 20.0f * (i + 1);
            drawNode->drawLine(Vec2(_colWidth[0], posY), Vec2(posX, posY), Color4F::BLACK);
        }
        for (int i = 0; i < 7; ++i) {
            const float posX = _posX[i] + _colWidth[i] * 0.5f;
            drawNode->drawLine(Vec2(posX, 0.0f), Vec2(posX, 80.0f), Color4F::BLACK);
        }
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *tableLabel = std::get<0>(ext);
    const std::array<std::array<Label *, 5>, 4> &labels = std::get<1>(ext);
    const std::array<ui::Button *, 2> &buttons = std::get<2>(ext);
    const std::array<LayerColor *, 2> &layerColors = std::get<3>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    const CompetitionTable &currentTable = _competitionTables->at(idx);
    tableLabel->setString(std::to_string(currentTable.serial + 1));  // 桌号

    const std::vector<CompetitionPlayer> &players = _competitionData->players;

    // 编号、选手姓名、顺位、标准分、比赛分，共6个Label
    for (int i = 0; i < 4; ++i) {
        ptrdiff_t playerIndex = currentTable.player_indices[i];
        if (playerIndex == INVALID_INDEX) {
            for (int k = 0; k < 5; ++k) {
                labels[i][k]->setString("");
            }
        }
        else {
            const CompetitionPlayer *player = &players[currentTable.player_indices[i]];
            labels[i][0]->setString(std::to_string(player->serial + 1));
            labels[i][1]->setString(player->name);
            labels[i][2]->setString(std::to_string(player->competition_results[_currentRound].rank));
            std::pair<float, int> ret = player->getCurrentScoresByRound(_currentRound);
            labels[i][3]->setString(CompetitionResult::standardScoreToString(ret.first));
            labels[i][4]->setString(std::to_string(ret.second));

            for (int k = 0; k < 5; ++k) {
                Common::scaleLabelToFitWidth(labels[i][k], _colWidth[2 + k] - 4.0f);
            }
        }
    }

    buttons[0]->setUserData(reinterpret_cast<void *>(idx));
    buttons[1]->setUserData(reinterpret_cast<void *>(idx));

    return cell;
}

void CompetitionTableScene::onClearButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t table = reinterpret_cast<size_t>(button->getUserData());
    const CompetitionTable &currentTable = _competitionTables->at(table);
    std::vector<CompetitionPlayer> &players = _competitionData->players;
    bool updateFlag = false;
    for (int i = 0; i < 4; ++i) {
        ptrdiff_t idx = currentTable.player_indices[i];
        if (idx == INVALID_INDEX) {
            continue;
        }

        CompetitionResult &result = players[idx].competition_results[_currentRound];
        result.rank = 0;
        result.standard_score = 0;
        result.competition_score = 0;
        updateFlag = true;
    }

    if (updateFlag) {
        // 刷新外面的UI
        _submitButton->setEnabled(_competitionData->isRoundFinished(_currentRound));
        _tableView->updateCellAtIndex(table);
        _competitionData->writeToFile(FileUtils::getInstance()->getWritablePath().append("competition.json"));
    }
}

void CompetitionTableScene::onRecordButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t table = reinterpret_cast<size_t>(button->getUserData());
    const CompetitionTable &currentTable = _competitionTables->at(table);

    // 有空位
    if (std::any_of(std::begin(currentTable.player_indices), std::end(currentTable.player_indices),
        [](ptrdiff_t idx) { return idx == INVALID_INDEX; })) {
        AlertView::showWithMessage("登记成绩", "请先排座位", 12, std::bind(&CompetitionTableScene::showRankAlert, this), nullptr);
        return;
    }

    CompetitionResult result[4];
    const std::vector<CompetitionPlayer> &players = _competitionData->players;
    for (int i = 0; i < 4; ++i) {
        result[i] = players[currentTable.player_indices[i]].competition_results[_currentRound];
    };

    showRecordAlert(table, result);
}

void CompetitionTableScene::showRecordAlert(size_t table, const CompetitionResult (&prevResult)[4]) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10.0f;
    const float height = 100;

    // 列宽
    const float colWidth[6] = {
        width * 0.1f,
        width * 0.1f,
        width * 0.26f,
        width * 0.18f,
        width * 0.18f,
        width * 0.18f,
    };
    std::array<float, 6> colWidthArray = { colWidth[0], colWidth[1], colWidth[2], colWidth[3], colWidth[4], colWidth[5] };

    // 中心位置
    float posX[6];
    Common::calculateColumnsCenterX(colWidth, 6, posX);

    // 根结点
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(width, height + 55.0f));

    // 表格画在DrawNode上
    DrawNode *drawNode = DrawNode::create();
    rootNode->addChild(drawNode);
    drawNode->setPosition(Vec2(0.0f, 55.0f));

    // shared_ptr随着AlertView构造，其他lambda捕获裸指针
    auto sharedResults = std::make_shared<std::array<CompetitionResult, 4> >(std::array<CompetitionResult, 4>({ prevResult[0], prevResult[1], prevResult[2], prevResult[3] }));
    std::array<CompetitionResult, 4> *results = sharedResults.get();

    // 检查
    Label *label = Label::createWithSystemFont("检查：标准分总和7，比赛分总和0", "Arail", 10);
    label->setColor(Color3B(254, 87, 110));
    rootNode->addChild(label);
    label->setPosition(Vec2(width * 0.5f, 45.0f));
    Common::scaleLabelToFitWidth(label, width - 4.0f);

    std::function<void (const std::array<CompetitionResult, 4> &)> refreshCheckLabel = [label](const std::array<CompetitionResult, 4> &results) {
        float ss = 0.0f;
        int cs = 0;
        for (int i = 0; i < 4; ++i) {
            ss += results[i].standard_score;
            cs += results[i].competition_score;
        }
        label->setString(Common::format("检查：标准分总和%.3f，比赛分总和%d", ss, cs));
    };

    // 横线
    for (int i = 0; i < 6; ++i) {
        const float y = static_cast<float>(i * 20);
        drawNode->drawLine(Vec2(0.0f, y), Vec2(width, y), Color4F::BLACK);
    }

    // 竖线
    drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(0.0f, 100.0f), Color4F::BLACK);
    for (int i = 0; i < 6; ++i) {
        const float x = posX[i] + colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(x, 0.0f), Vec2(x, 100.0f), Color4F::BLACK);
    }

    std::array<std::array<Label *, 6>, 4> labels;
    enum {
        SEAT, SERIAL, NAME, RANK, STANDARD_SCORE, COMPETITION_SCORE
    };

    static const char *titleTexts[] = { "座次", "编号", "选手姓名", "顺位", "标准分", "比赛分" };
    for (int i = 0; i < 6; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        drawNode->addChild(label);
        label->setPosition(Vec2(posX[i], 90.0f));
        Common::scaleLabelToFitWidth(label, colWidth[i] - 4.0f);
    }

    const std::vector<CompetitionPlayer> &players = _competitionData->players;
    const CompetitionTable &currentTable = _competitionTables->at(table);

    Color3B textColors[] = { Color3B::BLACK, Color3B(0x60, 0x60, 0x60), Color3B::ORANGE,
        Color3B(254, 87, 110), Color3B(44, 121, 178), Color3B(101, 196, 59) };
    for (int i = 0; i < 4; ++i) {
        const CompetitionPlayer *player = &players[currentTable.player_indices[i]];

        const float posY = 70.0f - 20.0f * i;

        std::string text[6] = { seatText[i], std::to_string(player->serial + 1), player->name, "", "", "" };

        for (int k = 0; k < 6; ++k) {
            Label *label = Label::createWithSystemFont(text[k], "Arail", 12);
            label->setColor(textColors[k]);
            drawNode->addChild(label);
            label->setPosition(Vec2(posX[k], posY));
            Common::scaleLabelToFitWidth(label, colWidth[k] - 4.0f);
            labels[i][k] = label;
        }

        const CompetitionResult *result = &results->at(i);

        // 刷新三个label的回调函数
        RefreshRecordAlertCallback callback = [labels, i, colWidthArray, results, refreshCheckLabel](const CompetitionResult &result) {
            std::string text[3] = {
                std::to_string(result.rank),
                CompetitionResult::standardScoreToString(result.standard_score),
                std::to_string(result.competition_score)
            };
            for (int k = 0; k < 3; ++k) {
                Label *label = labels[i][k + RANK];
                label->setString(text[k]);
                Common::scaleLabelToFitWidth(label, colWidthArray[k + RANK] - 4.0f);
            }
            results->at(i) = result;
            refreshCheckLabel(*results);
        };
        callback(*result);

        ui::Widget *widget = ui::Widget::create();
        widget->setTouchEnabled(true);
        widget->setPosition(Vec2(posX[4], posY));
        widget->setContentSize(Size(colWidth[4] * 3, 20.0f));
        drawNode->addChild(widget);
        widget->addClickEventListener([this, i, player, result, callback](Ref *) {
            std::string title = Common::format("「%s」：%" PRIzu " 「%s」", seatText[i], player->serial + 1, player->name.c_str());
            showCompetitionResultInputAlert(title, *result, callback);
        });
    }

    // 自动计算按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    rootNode->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("自动计算");
    button->setPosition(Vec2(width * 0.5f, 25.0f));
    button->addClickEventListener([this, labels](Ref *) {
        int scores[4] = {
            atoi(labels[0][COMPETITION_SCORE]->getString().c_str()),
            atoi(labels[1][COMPETITION_SCORE]->getString().c_str()),
            atoi(labels[2][COMPETITION_SCORE]->getString().c_str()),
            atoi(labels[3][COMPETITION_SCORE]->getString().c_str()) };
        int ranks[4];
        Common::calculateRankFromScore(scores, ranks);
        for (int i = 0; i < 4; ++i) {
            switch (ranks[i]) {
            case 0: labels[i][RANK]->setString("1"); labels[i][STANDARD_SCORE]->setString("4"); break;
            case 1: labels[i][RANK]->setString("2"); labels[i][STANDARD_SCORE]->setString("2"); break;
            case 2: labels[i][RANK]->setString("3"); labels[i][STANDARD_SCORE]->setString("1"); break;
            case 3: labels[i][RANK]->setString("4"); labels[i][STANDARD_SCORE]->setString("0"); break;
            default: break;
            }
        }
    });

    // 说明文本
    label = Label::createWithSystemFont("自动计算的标准分按4210计，不设并列，如需并列请手动输入", "Arail", 10);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    rootNode->addChild(label);
    label->setPosition(Vec2(width * 0.5f, 5.0f));
    Common::scaleLabelToFitWidth(label, width - 4.0f);

    std::string title = Common::format("第%" PRIzu "桌成绩", table + 1);
    AlertView::showWithNode(title, rootNode, [this, table, labels, title, sharedResults]() {
        std::array<CompetitionResult, 4> inputResult;
        for (int i = 0; i < 4; ++i) {
            const std::string &rank = labels[i][RANK]->getString();
            const std::string &ss = labels[i][STANDARD_SCORE]->getString();
            const std::string &cs = labels[i][COMPETITION_SCORE]->getString();

            CompetitionResult &result = inputResult[i];
            result.rank = atoi(rank.c_str());
            result.standard_score = static_cast<float>(atof(ss.c_str()));
            result.competition_score = atoi(cs.c_str());
        }

        if (std::any_of(std::begin(inputResult), std::end(inputResult),
            [](const CompetitionResult &result) { return result.rank == 0; })) {
            AlertView::showWithMessage(title, "请选择顺位或使用自动计算", 12, [this, table, inputResult]() {
                CompetitionResult result[4] = { inputResult[0], inputResult[1], inputResult[2], inputResult[3] };
                showRecordAlert(table, result);
            }, nullptr);
            return;
        }

        std::vector<CompetitionPlayer> &players = _competitionData->players;
        CompetitionTable &currentTable = _competitionTables->at(table);
        for (int i = 0; i < 4; ++i) {
            players[currentTable.player_indices[i]].competition_results[_currentRound] = inputResult[i];
        }

        // 刷新外面的UI
        _submitButton->setEnabled(_competitionData->isRoundFinished(_currentRound));
        _tableView->updateCellAtIndex(table);
        _competitionData->writeToFile(FileUtils::getInstance()->getWritablePath().append("competition.json"));
    }, nullptr);
}

void CompetitionTableScene::showCompetitionResultInputAlert(const std::string &title, const CompetitionResult &result, const RefreshRecordAlertCallback &callback) {
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(135.0f, 105.0f));

    Label *label = Label::createWithSystemFont("顺位", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 90.0f));

    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    radioGroup->setAllowedNoSelection(true);
    rootNode->addChild(radioGroup);
    for (int i = 0; i < 4; ++i) {
        ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(45.0f + i * 25, 90.0f));
        rootNode->addChild(radioButton);

        label = Label::createWithSystemFont(std::to_string(i + 1), "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioButton->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));

        radioGroup->addRadioButton(radioButton);
    }

    std::array<ui::EditBox *, 2> editBoxes;

    label = Label::createWithSystemFont("标准分", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 60.0f));

    ui::EditBox *editBox = ui::EditBox::create(Size(80.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
#else
    editBox->setInputMode(ui::EditBox::InputMode::DECIMAL);
#endif
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::NEXT);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(CompetitionResult::standardScoreToString(result.standard_score).c_str());
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(90.0f, 60.0f));
    editBoxes[0] = editBox;

    radioGroup->addEventListener([editBox](ui::RadioButton *, int index, ui::RadioButtonGroup::EventType) {
        switch (index) {
        case 0: editBox->setText("4"); break;
        case 1: editBox->setText("2"); break;
        case 2: editBox->setText("1"); break;
        case 3: editBox->setText("0"); break;
        default: break;
        }
    });

    label = Label::createWithSystemFont("比赛分", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 30.0f));

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", result.competition_score);

    editBox = ui::EditBox::create(Size(80.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
#else
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
#endif
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(buf);
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(90.0f, 30.0f));
    editBoxes[1] = editBox;

    // EditBox的代理
    auto delegate = std::make_shared<EditBoxDelegateWrapper>(std::vector<ui::EditBox *>(editBoxes.begin(), editBoxes.end()));
    editBoxes[0]->setDelegate(delegate.get());
    editBoxes[1]->setDelegate(delegate.get());

    Size visibleSize = Director::getInstance()->getVisibleSize();

    // 说明文本
    label = Label::createWithSystemFont("此处可仅输入比赛分，再使用「自动计算」", "Arail", 10);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    rootNode->addChild(label);
    label->setPosition(Vec2(135.0f * 0.5f, 5.0f));
    Common::scaleLabelToFitWidth(label, visibleSize.width * 0.8f - 14.0f);

    AlertView::showWithNode(title, rootNode, [this, radioGroup, editBoxes, title, result, callback, delegate]() {
        unsigned rank = 0;
        float standardScore = 0;
        int competitionScore = 0;

        rank = radioGroup->getSelectedButtonIndex() + 1;

        const char *text = editBoxes[0]->getText();
        if (*text != '\0') {
            standardScore = static_cast<float>(atof(text));
        }

        text = editBoxes[1]->getText();
        if (*text != '\0') {
            competitionScore = atoi(text);
        }

        CompetitionResult temp;
        temp.rank = rank;
        temp.standard_score = standardScore;
        temp.competition_score = competitionScore;
        callback(temp);
    }, nullptr);
}

void CompetitionTableScene::showRankAlert() {
    if (_competitionData->isRoundStarted(_currentRound)) {
        AlertView::showWithMessage("排列座位", "开始记录成绩后不允许重新排座位", 12, nullptr, nullptr);
        return;
    }

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(80.0f, 120.0f));

    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    rootNode->addChild(radioGroup);

    static const char *titles[] = { "随机", "编号蛇形", "排名蛇形", "高高碰", "自定义" };

    for (int i = 0; i < 5; ++i) {
        ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        rootNode->addChild(radioButton);
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(10.0f, 110.0f - i * 25.0f));
        radioGroup->addRadioButton(radioButton);

        Label *label = Label::createWithSystemFont(titles[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(25.0f, 110.0f - i * 25.0f));
    }

    AlertView::showWithNode("排列座位", rootNode, [this, radioGroup]() {
        switch (radioGroup->getSelectedButtonIndex()) {
        case 0: _competitionData->rankTablesByRandom(_currentRound); break;
        case 1: _competitionData->rankTablesBySerialSnake(_currentRound); break;
        case 2: _competitionData->rankTablesByScoresSnake(_currentRound); break;
        case 3: _competitionData->rankTablesByScores(_currentRound); break;
        case 4: Director::getInstance()->pushScene(CompetitionRankCustomScene::create(_competitionData, _currentRound)); return;
        default: return;
        }
        _tableView->reloadData();
        _competitionData->writeToFile(FileUtils::getInstance()->getWritablePath().append("competition.json"));
    }, nullptr);
}
