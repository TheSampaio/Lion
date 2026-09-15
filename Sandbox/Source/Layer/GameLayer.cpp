#include "GameLayer.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Filesystem.h>

using namespace Lion;

void GameLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();

	if (!SceneManager::LoadScene("Scenes/Splash.lnscene"))
		Log::Console(LogLevel::Fatal, "[Game] Could not load the entry scene.");
}

void GameLayer::OnUpdate()
{
	SceneManager::Update();
	UpdateMusic();
}

void GameLayer::OnRender()
{
	const Reference<Scene> scene = SceneManager::GetActiveScene();

	if (!scene)
		return;

	Camera2D* sceneCamera = scene->FindComponent<Camera2D>();
	PostProcessingComponent* postProcessing = nullptr;

	if (sceneCamera && sceneCamera->IsEnabled())
	{
		const glm::vec2 position = sceneCamera->GetViewPosition();
		mCamera->SetPosition(glm::vec3(position.x, position.y, 0.0f));

		mCamera->SetZoomLevel(sceneCamera->GetZoomForViewportHeight(mCamera->GetViewportHeight()));
		postProcessing = sceneCamera->GetOwner().GetComponent<PostProcessingComponent>();
	}

	const Size windowSize = Window::GetSize();
	const bool postProcessingActive = Renderer::BeginPostProcessing(postProcessing,
		static_cast<uint32>(windowSize.width), static_cast<uint32>(windowSize.height));
	Renderer::RenderBegin(mCamera);
	scene->OnRender();
	Renderer::RenderEnd();
	if (postProcessingActive)
		Renderer::EndPostProcessing();
}

void GameLayer::OnDetach()
{
	Audio::Stop(mMusicVoice);
	mMusicVoice = kInvalidAudioVoice;
	SceneManager::Clear();
}

void GameLayer::UpdateMusic()
{
	const std::string& path = SceneManager::GetActivePath();
	const bool gameplay = path.find("Scenes/Level") != std::string::npos;
	const bool menu = path.find("Scenes/MainMenu") != std::string::npos;

	if (!gameplay && !menu)
	{
		if (mMusicVoice != kInvalidAudioVoice)
		{
			Audio::Stop(mMusicVoice);
			mMusicVoice = kInvalidAudioVoice;
		}
		return;
	}

	if (mMusicVoice != kInvalidAudioVoice && Audio::IsPlaying(mMusicVoice)
		&& gameplay == mPlayingGameMusic)
		return;

	Audio::Stop(mMusicVoice);
	mPlayingGameMusic = gameplay;
	Reference<AudioClip>& clip = gameplay ? mGameMusic : mMenuMusic;
	const std::string clipPath = gameplay ? "Sounds/music-game.wav" : "Sounds/music-menu.wav";
	if (!clip)
		clip = Asset::LoadAudio(clipPath, clipPath);

	AudioPlayback playback;
	playback.volume = gameplay ? 0.42f : 0.36f;
	playback.loop = true;
	playback.bus = AudioBus::Music;
	mMusicVoice = Audio::Play(clip, playback, "brickout-music");
}

void GameLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowResize>(LION_BIND_EVENT(GameLayer::OnEventWindowResize));
}

bool GameLayer::OnEventWindowResize(const EventWindowResize& event)
{
	mCamera->OnResize(static_cast<float32>(event.GetWidth()), static_cast<float32>(event.GetHeight()));
	return false;
}
