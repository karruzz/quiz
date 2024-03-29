#include <voice.h>
#include <analyzer.h>
#include <utils.h>
#include <log.h>
#include <thread>
#include <functional>

#ifdef AUDIO_CAPTURE
#include <pocketsphinx/pocketsphinx.h>
#include <sphinxbase/ad.h>
#include <sphinxbase/err.h>
#include <sphinxbase/cmd_ln.h>
#include <cstdio>
#include <cstring>

#define SIZE 32000

static const char *hmm_verbose = "/usr/share/sphinx-voxforge-en/hmm/voxforge_en_sphinx.cd_cont_3000";
static const char *lm_verbose = "/usr/share/sphinx-voxforge-en/lm/voxforge_en_sphinx.cd_cont_3000/voxforge_en_sphinx.lm.DMP";
static const char *dict_verbose = "/usr/share/sphinx-voxforge-en/lm/voxforge_en_sphinx.cd_cont_3000/voxforge_en_sphinx.dic";

//static const char *hmm_default = "/usr/share/pocketsphinx/model/hmm/en_US/hub4wsj_sc_8k";
//static const char *lm_default = "/usr/share/pocketsphinx/model/lm/en_US/hub4.5000.DMP";
//static const char *dict_default = "/usr/share/pocketsphinx/model/lm/en_US/cmu07a.dic";
#endif

std::string AudioRecord::capture()
{
#ifdef AUDIO_CAPTURE
	ps_decoder_t *ps;                  // create pocketsphinx decoder structure
	cmd_ln_t *config;                  // create configuration structure
	ad_rec_t *ad;                      // create audio recording structure - for use with ALSA functions

	int16 adbuf[SIZE];                 // buffer array to hold audio data
	uint8 utt_started, in_speech;      // flags for tracking active speech - has speech started? - is speech currently happening?
	int32 k;                           // holds the number of frames in the audio buffer
	char const *decoded_speech;

	config = cmd_ln_init(NULL, ps_args(), TRUE,                   // Load the configuration structure - ps_args() passes the default values
	"-hmm", hmm_verbose,   // path to the standard english language model
	"-lm", lm_verbose,     // custom language model (file must be present)
	"-dict", dict_verbose, // custom dictionary (file must be present)
	"-logfn", "/dev/null", // suppress log info from being sent to screen
	NULL);

	ps = ps_init(config);                                                        // initialize the pocketsphinx decoder
	ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"), (int) cmd_ln_float32_r(config, "-samprate")); // open default microphone at default samplerate

	ad_start_rec(ad);    // start recording
	ps_start_utt(ps);    // mark the start of the utterance
	utt_started = FALSE; // clear the utt_started flag

	while(1) {
		k = ad_read(ad, adbuf, 4096);                // capture the number of frames in the audio buffer
		ps_process_raw(ps, adbuf, k, FALSE, FALSE);  // send the audio buffer to the pocketsphinx decoder

		in_speech = ps_get_in_speech(ps);            // test to see if speech is being detected

		if (in_speech && !utt_started)            // if speech has started and utt_started flag is false
			utt_started = TRUE;                   // then set the flag

		if (!in_speech && utt_started) {             // if speech has ended and the utt_started flag is true
			ps_end_utt(ps);                          // then mark the end of the utterance
			ad_stop_rec(ad);                         // stop recording
			decoded_speech = ps_get_hyp(ps, NULL);             // query pocketsphinx for "hypothesis" of decoded statement
			break;                                   // exit the while loop and return to main
		}
	}

	std::string result = decoded_speech;

	ad_close(ad);
	ps_free(ps);

	return result + " ";
#else
	return "";
#endif
}

static
void play_string(std::string s, utils::Language lang)
{
	try {
		if (s.empty()) return;

		std::list<analysis::Token> tokens = analysis::Analyzer::split_to_tokens(s);
		std::string expanded;
		for (const analysis::Token &l : tokens) {
			if (l.what != analysis::Token::WHAT::WORD)
				continue;

			std::string word = utils::to_utf8(l.str);
			if (word == "sb" || word == "smb") word = "somebody";
			else if (word == "sth" || word == "smth") word = "something";
			expanded.append(word + ' ');
		}

		static char audio_play_cmd[100];
#ifdef SPD_SAY
		sprintf(audio_play_cmd, "spd-say \"%s\"", expanded.c_str());
#elif defined GOOGLE_SPEECH
		utils::Language lan = lang != utils::Language::UNKNOWN
		                    ? lang
		                    : utils::what_language(utils::to_utf16(s));

		std::string l;
		switch(lan)
		{
			case (utils::Language::RU) :
				l = "ru";
				break;
			case (utils::Language::NL) :
				l = "nl";
				break;
			case (utils::Language::EN) :
			default:
				l = "en";
				break;
		}

		sprintf(audio_play_cmd, "google_speech -l %s \"%s\" > /dev/null 2>&1", l.c_str(), expanded.c_str());
#endif
		system(audio_play_cmd);
	} catch (const std::exception &e) {
		logging::Error() << "Exception in voice thread: " << e.what() << logging::endl;
	}
}

void AudioRecord::play(const std::string& phrase, utils::Language lang)
{
	std::thread t(std::bind(play_string, phrase, lang));
	t.detach();
}
