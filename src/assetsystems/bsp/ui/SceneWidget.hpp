#pragma once

#include <chrono>
#include <cstdint>
#include <source_location>
#include <unordered_map>
#include <vector>

#include <qnamespace.h>
#include <QOpenGLFunctions_4_5_Compatibility>
#include <QOpenGLWidget>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class BspFile;

struct FaceData
{
	GLuint Vbo{ 0 };
	GLuint Ibo{ 0 };
	std::uint16_t IndexCount{ 0 };
};

class SceneWidget final : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Compatibility
{
public:
	explicit SceneWidget(QWidget* parent = nullptr);
	~SceneWidget();

	void SetBspFile(BspFile* bspFile)
	{
		_currentBspFile = bspFile;
		_createObjects = true;

		_rotation = glm::vec2{ 0 };
		_translation = glm::vec3{ 0 };
	}

protected:
	void initializeGL() override;
	void paintGL() override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

private:
	bool HandleKeyChange(Qt::Key key, bool down);

	void CheckGLErrors(std::source_location location = std::source_location::current());

	void CreateBspObjects();
	void DestroyBspObjects();

	void DrawBspObjects();

private:
	GLuint _vao{ 0 };

	bool _createObjects{ false };

	BspFile* _currentBspFile{};

	std::vector<FaceData> _faces;

	std::vector<GLuint> _textures;

	glm::vec3 _translation{ 0 };
	glm::vec2 _rotation{ 0 };

	std::unordered_map<Qt::Key, bool> _keysDown;

	std::chrono::high_resolution_clock::time_point _lastUpdateTime;
};
