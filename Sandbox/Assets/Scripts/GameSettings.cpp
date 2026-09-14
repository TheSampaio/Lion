#include "GameSettings.h"

using namespace Lion;

namespace
{
	constexpr uint32 kWidths[] = { 960, 1280, 1600, 1920 };
	constexpr uint32 kHeights[] = { 540, 720, 900, 1080 };
	constexpr const char8* kLanguageNames[] = {
		"ENGLISH", "PORTUGUES", "ESPANOL", "ITALIANO",
		"FRANCAIS", "DEUTSCH", "RUSSKIY", "ELLINIKA"
	};

	using Translation = std::array<const char8*, static_cast<size_t>(GameText::Count)>;

	constexpr Translation kEnglish = {
		"PLAY", "CREDITS", "SETTINGS", "QUIT", "BACK", "SOUND", "RESOLUTION", "V-SYNC",
		"GRAPHICS", "BLOOM", "VIGNETTE", "MOTION BLUR", "CAMERA SHAKE", "COLOR MODE", "LANGUAGE",
		"ON", "OFF", "LOW", "MEDIUM", "HIGH", "NONE", "PROTANOPIA", "DEUTERANOPIA", "TRITANOPIA",
		"PAUSED", "RESUME", "MAIN MENU", "CIRCUIT SUSPENDED", "PLAY AGAIN", "TOTAL SCORE",
		"CIRCUIT CLEAR", "SYSTEM FAILURE", "SCORE", "BALLS", "LEVEL", "EXTRA BALL +1", "MULTIBALL X3",
		"PRESS ANY KEY TO START"
	};
	constexpr Translation kPortuguese = {
		"JOGAR", "CREDITOS", "CONFIGURACOES", "SAIR", "VOLTAR", "SOM", "RESOLUCAO", "V-SYNC",
		"GRAFICOS", "BLOOM", "VINHETA", "RASTRO DE MOVIMENTO", "TREMER CAMERA", "MODO DE COR", "IDIOMA",
		"LIGADO", "DESLIGADO", "BAIXA", "MEDIA", "ALTA", "NENHUM", "PROTANOPIA", "DEUTERANOPIA", "TRITANOPIA",
		"PAUSADO", "CONTINUAR", "MENU PRINCIPAL", "CIRCUITO SUSPENSO", "JOGAR DE NOVO", "PONTUACAO TOTAL",
		"CIRCUITO COMPLETO", "FALHA DO SISTEMA", "PONTOS", "BOLAS", "FASE", "BOLA EXTRA +1", "MULTIBOLA X3",
		"PRESSIONE UMA TECLA"
	};
	constexpr Translation kSpanish = {
		"JUGAR", "CREDITOS", "AJUSTES", "SALIR", "VOLVER", "SONIDO", "RESOLUCION", "V-SYNC",
		"GRAFICOS", "BLOOM", "VINETA", "DESENFOQUE", "SACUDIDA", "MODO DE COLOR", "IDIOMA",
		"SI", "NO", "BAJA", "MEDIA", "ALTA", "NINGUNO", "PROTANOPIA", "DEUTERANOPIA", "TRITANOPIA",
		"PAUSA", "CONTINUAR", "MENU PRINCIPAL", "CIRCUITO PAUSADO", "JUGAR DE NUEVO", "PUNTOS TOTALES",
		"CIRCUITO COMPLETO", "FALLO DEL SISTEMA", "PUNTOS", "BOLAS", "NIVEL", "BOLA EXTRA +1", "MULTIBOLA X3",
		"PULSA UNA TECLA"
	};
	constexpr Translation kItalian = {
		"GIOCA", "CREDITI", "IMPOSTAZIONI", "ESCI", "INDIETRO", "SUONO", "RISOLUZIONE", "V-SYNC",
		"GRAFICA", "BLOOM", "VIGNETTA", "MOSSO", "SCOSSA CAMERA", "MODALITA COLORE", "LINGUA",
		"SI", "NO", "BASSA", "MEDIA", "ALTA", "NESSUNO", "PROTANOPIA", "DEUTERANOPIA", "TRITANOPIA",
		"PAUSA", "RIPRENDI", "MENU PRINCIPALE", "CIRCUITO SOSPESO", "GIOCA ANCORA", "PUNTEGGIO TOTALE",
		"CIRCUITO COMPLETO", "ERRORE DI SISTEMA", "PUNTI", "PALLE", "LIVELLO", "PALLA EXTRA +1", "MULTIPALLA X3",
		"PREMI UN TASTO"
	};
	constexpr Translation kFrench = {
		"JOUER", "CREDITS", "PARAMETRES", "QUITTER", "RETOUR", "SON", "RESOLUTION", "V-SYNC",
		"GRAPHISMES", "BLOOM", "VIGNETTE", "FLOU DE MOUVEMENT", "SECOUSSE CAMERA", "MODE COULEUR", "LANGUE",
		"OUI", "NON", "FAIBLE", "MOYEN", "ELEVE", "AUCUN", "PROTANOPIE", "DEUTERANOPIE", "TRITANOPIE",
		"PAUSE", "REPRENDRE", "MENU PRINCIPAL", "CIRCUIT SUSPENDU", "REJOUER", "SCORE TOTAL",
		"CIRCUIT TERMINE", "PANNE SYSTEME", "SCORE", "BALLES", "NIVEAU", "BALLE BONUS +1", "MULTIBALLE X3",
		"APPUYEZ SUR UNE TOUCHE"
	};
	constexpr Translation kGerman = {
		"SPIELEN", "CREDITS", "EINSTELLUNGEN", "BEENDEN", "ZURUCK", "TON", "AUFLOSUNG", "V-SYNC",
		"GRAFIK", "BLOOM", "VIGNETTE", "BEWEGUNGSUNSCHARFE", "KAMERAWACKELN", "FARBMODUS", "SPRACHE",
		"AN", "AUS", "NIEDRIG", "MITTEL", "HOCH", "KEIN", "PROTANOPIE", "DEUTERANOPIE", "TRITANOPIE",
		"PAUSE", "FORTSETZEN", "HAUPTMENU", "KREISLAUF PAUSIERT", "NOCHMAL SPIELEN", "GESAMTPUNKTE",
		"KREISLAUF FREI", "SYSTEMFEHLER", "PUNKTE", "BALLE", "LEVEL", "EXTRA BALL +1", "MULTIBALL X3",
		"TASTE DRUCKEN"
	};
	constexpr Translation kRussian = {
		"IGRAT", "AVTORY", "NASTROYKI", "VYHOD", "NAZAD", "ZVUK", "RAZRESHENIE", "V-SYNC",
		"GRAFIKA", "BLOOM", "VINETKA", "RAZMytIE", "TRYASKA KAMERY", "REZHIM CVETA", "YAZYK",
		"VKL", "VYKL", "NIZKO", "SREDNE", "VYSOKO", "NET", "PROTANOPIA", "DEUTERANOPIA", "TRITANOPIA",
		"PAUZA", "PRODOLZHIT", "GLAVNOE MENU", "CEPI PRIostanovleny", "SNOVA IGRAT", "OBSHIY SCHET",
		"CEPI OCHISHENY", "SBOY SISTEMY", "SCHET", "MYACHI", "UROVEN", "DOP MYACH +1", "MULTIMYACH X3",
		"NAZHMITE KLAVISHU"
	};
	constexpr Translation kGreek = {
		"PAIXE", "SYNTELESTES", "RYTHMISEIS", "EXODOS", "PISO", "HXOS", "ANALYSI", "V-SYNC",
		"GRAFIKA", "BLOOM", "VIGNETTA", "THOLWMA KINISIS", "KOUNIMA KAMERAS", "LEITOURGIA XROMATOS", "GLOSSA",
		"NAI", "OXI", "XAMILA", "MESAIA", "YPSILA", "KANENA", "PROTANOPIA", "DEUTERANOPIA", "TRITANOPIA",
		"PAUSI", "SYNEXEIA", "KYRIO MENOY", "KYKLOMA SE PAUSI", "PAIXE XANA", "SYNOLO PONTON",
		"KYKLOMA KATHARO", "SFALMA SYSTIMATOS", "PONTOI", "BALES", "EPIPEDO", "EXTRA BALA +1", "POLYBALO X3",
		"PATISE ENA PLIKTRO"
	};

	constexpr const Translation* kTranslations[] = {
		&kEnglish, &kPortuguese, &kSpanish, &kItalian,
		&kFrench, &kGerman, &kRussian, &kGreek
	};
}

int32 GameSettings::Wrap(int32 value, int32 count)
{
	return (value % count + count) % count;
}

void GameSettings::Change(int32 setting, int32 direction)
{
	direction = direction < 0 ? -1 : 1;
	switch (setting)
	{
		case 0: sSound = !sSound; Audio::SetBusVolume(AudioBus::Master, sSound ? 1.0f : 0.0f); break;
		case 1: sResolution = Wrap(sResolution + direction, static_cast<int32>(std::size(kWidths))); ApplyWindow(); break;
		case 2: sVSync = !sVSync; ApplyWindow(); break;
		case 3: sQuality = Wrap(sQuality + direction, 3); break;
		case 4: sBloom = !sBloom; break;
		case 5: sVignette = !sVignette; break;
		case 6: sMotionBlur = !sMotionBlur; break;
		case 7: sCameraShake = !sCameraShake; break;
		case 8: sColorMode = Wrap(sColorMode + direction, 4); break;
		case 9: sLanguage = Wrap(sLanguage + direction, static_cast<int32>(std::size(kLanguageNames))); break;
	}
}

void GameSettings::ApplyWindow()
{
	Window::SetSize(kWidths[sResolution], kHeights[sResolution]);
	Graphics::SetVerticalSynchronization(sVSync);
}

void GameSettings::Apply(PostProcessingComponent& postProcessing)
{
	// Quality scales effect cost and intensity without overriding the player's individual toggles.
	postProcessing.SetBloom(sBloom);
	postProcessing.SetVignette(sVignette);
	postProcessing.SetMotionBlur(sMotionBlur);
	postProcessing.SetColorVisionMode(sColorMode);
	postProcessing.SetBloomStrength(sQuality == 0 ? 0.45f : sQuality == 1 ? 0.68f : 0.9f);
	postProcessing.SetVignetteStrength(sQuality == 0 ? 0.12f : sQuality == 1 ? 0.18f : 0.24f);
	postProcessing.SetMotionBlurStrength(sQuality == 0 ? 0.08f : sQuality == 1 ? 0.14f : 0.2f);
	postProcessing.SetChromaticAberration(sQuality > 0);
}

const char* GameSettings::Text(GameText text)
{
	return (*kTranslations[sLanguage])[static_cast<size_t>(text)];
}

std::string GameSettings::Label(int32 setting)
{
	const char8* state = nullptr;
	switch (setting)
	{
		case 0: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::Sound), Text(sSound ? GameText::On : GameText::Off));
		case 1: return LION_FORMAT_TEXT("{}  < {} X {} >", Text(GameText::Resolution), kWidths[sResolution], kHeights[sResolution]);
		case 2: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::VSync), Text(sVSync ? GameText::On : GameText::Off));
		case 3:
			state = Text(sQuality == 0 ? GameText::Low : sQuality == 1 ? GameText::Medium : GameText::High);
			return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::Quality), state);
		case 4: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::Bloom), Text(sBloom ? GameText::On : GameText::Off));
		case 5: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::Vignette), Text(sVignette ? GameText::On : GameText::Off));
		case 6: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::MotionBlur), Text(sMotionBlur ? GameText::On : GameText::Off));
		case 7: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::CameraShake), Text(sCameraShake ? GameText::On : GameText::Off));
		case 8:
			state = Text(sColorMode == 0 ? GameText::None : sColorMode == 1 ? GameText::Protanopia
				: sColorMode == 2 ? GameText::Deuteranopia : GameText::Tritanopia);
			return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::ColorMode), state);
		case 9: return LION_FORMAT_TEXT("{}  < {} >", Text(GameText::Language), kLanguageNames[sLanguage]);
	}
	return {};
}
