#include <cassert>
#include <stdexcept>

#include <QKeyEvent>
#include <QMessageBox>
#include <QWheelEvent>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "formats/bsp/BspFile.hpp"

#include "assetsystems/bsp/ui/SceneWidget.hpp"

SceneWidget::SceneWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &SceneWidget::frameSwapped, this, qOverload<>(&SceneWidget::update));

	_lastUpdateTime = std::chrono::high_resolution_clock::now();
}

SceneWidget::~SceneWidget()
{
	if (_vao)
	{
		DestroyBspObjects();
		glDeleteVertexArrays(1, &_vao);
	}
}

void SceneWidget::initializeGL()
{
	if (!initializeOpenGLFunctions())
	{
		throw std::runtime_error("Couldn't initialize OpenGL functions");
	}

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);
}

void SceneWidget::paintGL()
{
	glClearColor(0.5f, 0.5f, 0.5f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (_createObjects)
	{
		_createObjects = false;

		assert(_currentBspFile);

		DestroyBspObjects();
		CreateBspObjects();
	}

	glViewport(0, 0, width(), height());
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(glm::perspective(90.f, (float)width() / height(), 1.f, (float)(1 << 16))));

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	const auto now = std::chrono::high_resolution_clock::now();

	const auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastUpdateTime);

	{
		const int degreesPerSecond = 90;

		const float deltaThisFrame = (deltaTime.count() / 1000.f) * degreesPerSecond;

		if (auto it = _keysDown.find(Qt::Key::Key_Left); it != _keysDown.end() && it->second)
		{
			_rotation.y -= deltaThisFrame;
		}

		if (auto it = _keysDown.find(Qt::Key::Key_Right); it != _keysDown.end() && it->second)
		{
			_rotation.y += deltaThisFrame;
		}

		if (auto it = _keysDown.find(Qt::Key::Key_Up); it != _keysDown.end() && it->second)
		{
			_rotation.x -= deltaThisFrame;
		}

		if (auto it = _keysDown.find(Qt::Key::Key_Down); it != _keysDown.end() && it->second)
		{
			_rotation.x += deltaThisFrame;
		}

		_lastUpdateTime = now;
	}

	glm::mat4x4 modelMatrix = glm::identity<glm::mat4x4>();

	modelMatrix = glm::translate(modelMatrix, -_translation);

	const glm::vec3 angles{ -_rotation.x, -_rotation.y, 0 };

	const glm::quat rotation{ glm::vec3{glm::radians(angles.z), glm::radians(-angles.x), glm::radians(angles.y)} };

	modelMatrix *= glm::mat4_cast(rotation);

	glm::mat4x4 viewMatrix = glm::lookAt(_translation, _translation + glm::normalize(glm::vec3(modelMatrix[0])), glm::normalize(glm::vec3(modelMatrix[2])));

	glLoadMatrixf(glm::value_ptr(viewMatrix));
	
	if (_currentBspFile)
	{
		DrawBspObjects();
	}
}

void SceneWidget::keyPressEvent(QKeyEvent* event)
{
	if (!HandleKeyChange((Qt::Key)event->key(), true))
	{
		QOpenGLWidget::keyPressEvent(event);
	}
}

void SceneWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (!HandleKeyChange((Qt::Key)event->key(), false))
	{
		QOpenGLWidget::keyReleaseEvent(event);
	}
}

bool SceneWidget::HandleKeyChange(Qt::Key key, bool down)
{
	switch (key)
	{
	case Qt::Key::Key_Left:
		[[fallthrough]];
	case Qt::Key::Key_Right:
		[[fallthrough]];
	case Qt::Key::Key_Up:
		[[fallthrough]];
	case Qt::Key::Key_Down:
	{
		_keysDown[key] = down;
		return true;
	}
	}

	return false;
}

void SceneWidget::wheelEvent(QWheelEvent* event)
{
	if (const QPoint degrees = event->angleDelta() / 8; !degrees.isNull())
	{
		const int delta = 20;

		glm::mat4x4 modelMatrix = glm::identity<glm::mat4x4>();

		modelMatrix = glm::translate(modelMatrix, -_translation);

		const glm::vec3 angles{ -_rotation.x, -_rotation.y, 0 };

		const glm::quat rotation{ glm::vec3{glm::radians(angles.z), glm::radians(-angles.x), glm::radians(angles.y)} };

		modelMatrix *= glm::mat4_cast(rotation);

		_translation += glm::normalize(glm::vec3(modelMatrix[0])) * ((degrees.y() / 15.f) * delta);
	}
}

void SceneWidget::CheckGLErrors(std::source_location location)
{
	const GLenum error = glGetError();

	if (error == GL_NO_ERROR)
	{
		return;
	}

	QMessageBox::critical(this, "OpenGL Error", QString{"OpenGL error %1 (0X%2)"}.arg(error).arg(error, 0, 16));
}

void SceneWidget::CreateBspObjects()
{
	std::vector<std::uint16_t> indices;

	for (auto& face : _currentBspFile->Faces)
	{
		FaceData data;

		glGenBuffers(1, &data.Vbo);
		glGenBuffers(1, &data.Ibo);

		glBindBuffer(GL_ARRAY_BUFFER, data.Vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.Ibo);

		indices.clear();

		for (std::uint16_t i = 0; const auto& vertex : face.Vertexes)
		{
			indices.push_back(i);
			++i;
		}

		glNamedBufferData(data.Vbo, sizeof(glm::vec3) * face.Vertexes.size(), face.Vertexes.data(), GL_STATIC_DRAW);
		glNamedBufferData(data.Ibo, sizeof(std::uint16_t) * indices.size(), indices.data(), GL_STATIC_DRAW);

		data.IndexCount = face.Vertexes.size();

		_faces.push_back(data);
	}

	CheckGLErrors();

	_textures.resize(_currentBspFile->Textures.size());

	std::vector<std::uint8_t> pixels;

	for (std::size_t i = 0; i < _currentBspFile->Textures.size(); ++i)
	{
		auto& textureId = _textures[i];
		const auto texture = &_currentBspFile->Textures[i];

		glGenTextures(1, &textureId);

		CheckGLErrors();

		glBindTexture(GL_TEXTURE_2D, textureId);

		CheckGLErrors();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		const auto& sourceData = texture->TextureDatas[0];

		if (!sourceData.empty())
		{
			pixels.resize(sourceData.size() * 4);

			for (std::size_t pixelIndex = 0; pixelIndex < sourceData.size(); ++pixelIndex)
			{
				const auto& color = texture->Colormap[sourceData[pixelIndex]];

				pixels[(pixelIndex * 4) + 0] = color.R;
				pixels[(pixelIndex * 4) + 1] = color.G;
				pixels[(pixelIndex * 4) + 2] = color.B;
				pixels[(pixelIndex * 4) + 3] = 0xFF;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->Width, texture->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		}
		else
		{
			pixels.resize(16);

			pixels[0] = 0xFF;
			pixels[1] = 0;
			pixels[2] = 0xFF;
			pixels[3] = 0xFF;

			pixels[4] = 0;
			pixels[5] = 0;
			pixels[6] = 0;
			pixels[7] = 0xFF;

			pixels[8] = 0;
			pixels[9] = 0;
			pixels[10] = 0;
			pixels[11] = 0xFF;

			pixels[12] = 0xFF;
			pixels[13] = 0;
			pixels[14] = 0xFF;
			pixels[15] = 0xFF;

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		}

		CheckGLErrors();
	}
}

void SceneWidget::DestroyBspObjects()
{
	for (auto& textureId : _textures)
	{
		glDeleteTextures(1, &textureId);
	}

	_textures.clear();

	for (auto& face : _faces)
	{
		glDeleteBuffers(1, &face.Ibo);
		glDeleteBuffers(1, &face.Vbo);
	}

	_faces.clear();
}

constexpr glm::vec3 Colors[]
{
	{1.f, 0.f, 0.f},
	{0.f, 1.f, 0.f},
	{0.f, 0.f, 1.f},
	{1.f, 1.f, 0.f},
	{0.f, 1.f, 1.f},
	{1.f, 0.f, 1.f},
	{0.5f, 0.5f, 0.5f}
};

void SceneWidget::DrawBspObjects()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glColor4f(1.f, 1.f, 1.f, 1.f);

	auto& worldModel = _currentBspFile->Models[0];

	for (std::size_t colorIndex = 0; const auto& face : worldModel.Faces)
	{
		const auto& color = Colors[colorIndex % std::size(Colors)];

		const float adjust = std::sin(colorIndex) * 0.25;

		//glColor3f(color.x + adjust, color.y + adjust, color.z + adjust);

		const auto textureInfo = face.TextureInfo;

		const std::ptrdiff_t textureIndex = textureInfo->Texture - _currentBspFile->Textures.data();

		glBindTexture(GL_TEXTURE_2D, _textures[textureIndex]);

		CheckGLErrors();

		glBegin(GL_TRIANGLE_FAN);
		
		for (const auto& vertex : face.Vertexes)
		{
			const float s = (glm::dot(vertex, textureInfo->Vertices[0]) + textureInfo->STCoordinates[0]) / textureInfo->Texture->Width;
			const float t = (glm::dot(vertex, textureInfo->Vertices[1]) + textureInfo->STCoordinates[1]) / textureInfo->Texture->Height;

			glTexCoord2f(s, t);
			glVertex3f(vertex.x, vertex.y, vertex.z);
		}

		glEnd();

		CheckGLErrors();

		++colorIndex;
	}
}
