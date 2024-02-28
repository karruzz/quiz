/*
 * voice.h
 *
 *  Created on: May 19, 2018
 *  Copyright © 2018-2081 Ilja Karasev <ilja.karasev@gmail.com>.
 *  All rights reserved.
 *     License: GNU GPL 3
 */

#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <utils.h>

class AudioRecord {
public:
	static std::string capture();
	static void play(const std::string& phrase, utils::Language lang = utils::Language::UNKNOWN);
};

#endif // RECORD_H


