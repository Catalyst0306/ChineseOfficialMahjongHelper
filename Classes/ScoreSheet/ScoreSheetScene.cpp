﻿#include "ScoreSheetScene.h"
#include "RecordScene.h"

#pragma execution_character_set("utf-8")

USING_NS_CC;

static char g_name[4][255];
static int g_scores[16][4];
static int g_currentIndex;

Scene *ScoreSheetScene::createScene() {
    auto scene = Scene::create();
    auto layer = ScoreSheetScene::create();
    scene->addChild(layer);
    return scene;
}

bool ScoreSheetScene::init() {
    if (!Layer::init()) {
        return false;
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    memset(_scores, 0, sizeof(_scores));

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont("麻将竞赛成绩记录表", "Arial", 24);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));

    ui::Button *button = ui::Button::create();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("重置");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - tileLabel->getContentSize().height - 10));
    button->addClickEventListener([this](Ref *) { reset(); });

    Node *node = Node::create();
    this->addChild(node);
    node->setPosition(Vec2(origin.x, (origin.y + visibleSize.height - 430) * 0.5f));

    const float gap = visibleSize.width / 6;

    for (int i = 0; i < 5; ++i) {
        LayerColor *vertLine = LayerColor::create(Color4B(255, 255, 255, 255), 2, 400);
        node->addChild(vertLine);
        vertLine->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
        vertLine->setPosition(Vec2(gap * (i + 1), 0.0f));
    }

    for (int i = 0; i < 21; ++i) {
        LayerColor *horzLine = LayerColor::create(Color4B(255, 255, 255, 255), visibleSize.width, 2);
        node->addChild(horzLine);
        horzLine->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        horzLine->setPosition(Vec2(0.0f, 20.0f * i));
    }

    Label *label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setPosition(Vec2(gap * 0.5f, 390));
    node->addChild(label);

    for (int i = 0; i < 4; ++i) {
        _editBox[i] = ui::EditBox::create(Size(gap, 20.0f), ui::Scale9Sprite::create());
        _editBox[i]->setPosition(Vec2(gap * (i + 1.5f), 390));
        node->addChild(_editBox[i]);
    }

    _lockButton = ui::Button::create();
    node->addChild(_lockButton);
    _lockButton->setScale9Enabled(true);
    _lockButton->setContentSize(Size(gap, 20.0f));
    _lockButton->setTitleFontSize(12);
    _lockButton->setTitleText("锁定");
    _lockButton->setPosition(Vec2(gap * 5.5f, 390));
    _lockButton->addClickEventListener(std::bind(&ScoreSheetScene::lockCallback, this, std::placeholders::_1));

    const char *row0Text[] = {"开局座位", "东", "南", "西", "北"};
    const char *row1Text[] = {"每圈座位", "东南北西", "南东西北", "西北东南", "北西南东"};
    for (int i = 0; i < 5; ++i) {
        label = Label::createWithSystemFont(row0Text[i], "Arail", 12);
        label->setPosition(Vec2(gap * (i + 0.5f), 370));
        node->addChild(label);

        label = Label::createWithSystemFont(row1Text[i], "Arail", 12);
        label->setPosition(Vec2(gap * (i + 0.5f), 350));
        node->addChild(label);
    }

    label = Label::createWithSystemFont("累计", "Arail", 12);
    label->setPosition(Vec2(gap * 0.5f, 330));
    node->addChild(label);

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i] = Label::createWithSystemFont("0", "Arail", 12);
        _totalLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 330));
        node->addChild(_totalLabel[i]);
    }

    const char *nameText[] = {"东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
        "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北"};
    for (int k = 0; k < 16; ++k) {
        const float y = 10 + (15 - k) * 20;
        label = Label::createWithSystemFont(nameText[k], "Arail", 12);
        label->setPosition(Vec2(gap * 0.5f, y));
        node->addChild(label);

        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i] = Label::createWithSystemFont("", "Arail", 12);
            _scoreLabels[k][i]->setPosition(Vec2(gap * (i + 1.5f), y));
            node->addChild(_scoreLabels[k][i]);
        }

        _recordButton[k] = ui::Button::create();
        node->addChild(_recordButton[k]);
        _recordButton[k]->setScale9Enabled(true);
        _recordButton[k]->setContentSize(Size(gap, 20.0f));
        _recordButton[k]->setTitleFontSize(12);
        _recordButton[k]->setTitleText("计分");
        _recordButton[k]->setPosition(Vec2(gap * 5.5f, y));
        _recordButton[k]->addClickEventListener(std::bind(&ScoreSheetScene::recordCallback, this, std::placeholders::_1, k));

        _recordButton[k]->setVisible(false);
    }

    recover();
    return true;
}

void ScoreSheetScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->popScene();
    }
}

void ScoreSheetScene::recover() {
    bool empty = false;
    for (int i = 0; i < 4; ++i) {
        if (g_name[i][0] == '\0') {
            empty = true;
            break;
        }
    }

    if (empty) {
        memset(g_scores, 0, sizeof(g_scores));
        g_currentIndex = 0;
        return;
    }

    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setText(g_name[i]);
        _editBox[i]->setEnabled(false);
        _totalLabel[i]->setString("+0");
        _scoreLabels[0][i]->setString("+0");
    }

    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    memset(_scores, 0, sizeof(_scores));
    for (int k = 0; k < g_currentIndex; ++k) {
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i]->setString(StringUtils::format("%+d", g_scores[k][i]));
            _scores[i] += g_scores[k][i];

        }

        _recordButton[k]->setVisible(false);
        _recordButton[k]->setEnabled(false);
    }

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i]->setString(StringUtils::format("%+d", _scores[i]));
    }

    if (g_currentIndex < 16) {
        _recordButton[g_currentIndex]->setVisible(true);
        _recordButton[g_currentIndex]->setEnabled(true);
    }
}

void ScoreSheetScene::reset() {
    memset(g_name, 0, sizeof(g_name));
    memset(g_scores, 0, sizeof(g_scores));
    g_currentIndex = 0;

    memset(_scores, 0, sizeof(_scores));
    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setText("");
        _editBox[i]->setEnabled(true);
        _totalLabel[i]->setString("0");
    }

    _lockButton->setEnabled(true);
    _lockButton->setVisible(true);

    for (int k = 0; k < 16; ++k) {
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i]->setString("");
        }
        _recordButton[k]->setVisible(false);
        _recordButton[k]->setEnabled(false);
    }
}

void ScoreSheetScene::lockCallback(cocos2d::Ref *sender) {
    for (int i = 0; i < 4; ++i) {
        const char *str = _editBox[i]->getText();
        if (str[0] == '\0') {
            return;
        }
        strncpy(g_name[i], str, sizeof(g_name[i]));
    }

    memset(_scores, 0, sizeof(_scores));

    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setEnabled(false);
    }

    _recordButton[0]->setVisible(true);
    _recordButton[0]->setEnabled(true);
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);
}

void ScoreSheetScene::recordCallback(cocos2d::Ref *sender, int index) {
    const char *name[] = { _editBox[0]->getText(), _editBox[1]->getText(), _editBox[2]->getText(), _editBox[3]->getText() };
    Director::getInstance()->pushScene(RecordScene::createScene(index, name, [this, index](const int (&scores)[4]) {
        for (int i = 0; i < 4; ++i) {
            g_scores[index][i] = scores[i];
            _scoreLabels[index][i]->setString(StringUtils::format("%+d", scores[i]));
            _scores[i] += scores[i];
            _totalLabel[i]->setString(StringUtils::format("%+d", _scores[i]));
        }

        _recordButton[g_currentIndex]->setVisible(false);
        _recordButton[g_currentIndex]->setEnabled(false);
        if (++g_currentIndex < 16) {
            _recordButton[g_currentIndex]->setVisible(true);
            _recordButton[g_currentIndex]->setEnabled(true);
        }
    }));
}
