#include "gameapplication.h"
#include "gl/scenenode.h"
#include "nodes/coloredcube.h"
#include "json/scenejsonparser.h"

static QString SCENE_JSON_PATH = "D:/CG2015lab/02_lab/scene.json";
const QSize FIXED_WINDOW_SIZE(800, 600);

GameApplication::GameApplication(int argc, char *argv[])
    : QGuiApplication(argc, argv)
{
}

int GameApplication::enterGameLoop()
{
    m_window.setFixedSize(FIXED_WINDOW_SIZE);
    m_window.show();
    //connect(&m_window, SIGNAL(activeChanged()), this, SLOT(loadScene()));
    connect(&m_window, SIGNAL(activeChanged()), this, SLOT(loadSceneFromJson()));

    return exec();
}

void GameApplication::loadScene()
{
    disconnect(&m_window, SIGNAL(activeChanged()), this, SLOT(loadScene()));

    auto scene = std::make_shared<GameScene>();
    scene->camera().setViewport(m_window.size());
    scene->camera().lookAt(QVector3D(6, 3, 2), QVector3D(0, 0, 0), QVector3D(0, 0, 1));
    scene->prepareControllers();

    new ColoredCube(scene.get());

    m_window.pushScene(scene);
}

void GameApplication::loadSceneFromJson()
{
    disconnect(&m_window, SIGNAL(activeChanged()), this, SLOT(loadSceneFromJson()));

    auto scene = std::make_shared<GameScene>();
    scene->camera().setViewport(m_window.size());
    auto parser = new SceneJsonParser(scene, SCENE_JSON_PATH);
    parser->readIntoJsonObject();
    parser->setCameraSettings();
    scene->prepareControllers();
    parser->parseObjects();

    if (parser->isValid())
    {
        m_window.pushScene(scene);
    }
}



