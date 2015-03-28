/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2008-2013  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "klick.hh"
#include "audio_interface.hh"
#include "terminal_handler.hh"
#include "metronome_simple.hh"

#include <iostream>
#include <iomanip>
#include <cstdio>

#include <sys/select.h>
#include <unistd.h>

#include "util/debug.hh"


typedef std::shared_ptr<MetronomeSimple> MetronomeSimplePtr;


TerminalHandler::TerminalHandler(Klick & klick, AudioInterface & audio)
  : _klick(klick)
  , _audio(audio)
{
    // save current terminal mode
    ::tcgetattr(STDIN_FILENO, &_old_mode);

    // turn off line buffering
    ::termios term;
    ::tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    ::tcsetattr(STDIN_FILENO, TCSANOW, &term);
    ::setbuf(stdin, NULL);

    // hide cursor
    std::cout << "\033[?25l";

    update();
}


TerminalHandler::~TerminalHandler()
{
    // restore previous terminal mode
    ::tcsetattr(STDIN_FILENO, TCSANOW, &_old_mode);

    // show cursor again
    std::cout << "\033[?25h";

    std::cout << std::endl;
}


bool TerminalHandler::key_pressed()
{
    if (!::isatty(STDIN_FILENO)) {
        return false;
    }
    ::timeval timeout = { 0, 0 };
    ::fd_set rdset;
    FD_ZERO(&rdset);
    FD_SET(STDIN_FILENO, &rdset);

    return ::select(STDIN_FILENO + 1, &rdset, NULL, NULL, &timeout);
}


int TerminalHandler::get_key()
{
    return std::fgetc(stdin);
}


int TerminalHandler::peek_key()
{
    if (key_pressed()) {
        int k = std::fgetc(stdin);
        std::ungetc(k, stdin);
        return k;
    } else {
        return 0;
    }
}


bool TerminalHandler::munch_key(int k)
{
    if (peek_key() == k) {
        get_key();
        return true;
    } else {
        return false;
    }
}


void TerminalHandler::handle_input()
{
    while (key_pressed())
    {
        switch (get_key()) {
            case 033:
                if (key_pressed() && get_key() == '[' && key_pressed()) {
                    // escape sequence
                    switch (get_key()) {
                        // arrow keys: tempo
                        case 'A': change_tempo(+10); break;
                        case 'B': change_tempo(-10); break;
                        case 'C': change_tempo(+1); break;
                        case 'D': change_tempo(-1); break;
                        // pgup/pgdn: double/halve tempo
                        case '5': if (munch_key('~')) multiply_tempo(2.0f); break;
                        case '6': if (munch_key('~')) multiply_tempo(0.5f); break;
                        default:
                            // unknown escape sequence
                            while (key_pressed()) get_key();
                            break;
                    }
                }
                else {
                    // escape key
                    _klick.signal_quit();
                }
                break;

            // beats
            case '0': set_beats(0); break;
            case '1': set_beats(1); break;
            case '2': set_beats(2); break;
            case '3': set_beats(3); break;
            case '4': set_beats(4); break;
            case '5': set_beats(5); break;
            case '6': set_beats(6); break;
            case '7': set_beats(7); break;
            case '8': set_beats(8); break;
            case '9': set_beats(9); break;

            // denominator
            case 'q': set_denom(1); break;
            case 'w': set_denom(2); break;
            case 'e': set_denom(4); break;
            case 'r': set_denom(8); break;

            // volume
            case '+': case '=': change_volume(+0.1); break;
            case '-': change_volume(-0.1); break;

            case ' ':
                toggle_running();
                break;

            default:
                break;
        }

        update();
    }
}


void TerminalHandler::set_beats(int n)
{
    MetronomeSimplePtr m = std::dynamic_pointer_cast<MetronomeSimple>(_klick.metronome());

    m->set_meter(n, m->denom());
}


void TerminalHandler::set_denom(int n)
{
    MetronomeSimplePtr m = std::dynamic_pointer_cast<MetronomeSimple>(_klick.metronome());

    m->set_meter(m->beats(), n);
}


void TerminalHandler::change_tempo(float f)
{
    MetronomeSimplePtr m = std::dynamic_pointer_cast<MetronomeSimple>(_klick.metronome());

    float t = m->tempo() + f;
    t = std::min(std::max(t, 10.0f), 1000.0f);
    m->set_tempo(t);
}


void TerminalHandler::multiply_tempo(float f)
{
    MetronomeSimplePtr m = std::dynamic_pointer_cast<MetronomeSimple>(_klick.metronome());

    float t = m->tempo() * f;
    t = std::min(std::max(t, 10.0f), 1000.0f);
    m->set_tempo(t);
}


void TerminalHandler::change_volume(float f)
{
    float v = _audio.volume() + f;
    v = std::min(std::max(v, 0.0f), 2.0f);
    _audio.set_volume(v);
}


void TerminalHandler::toggle_running()
{
    bool active = _klick.metronome()->active();
    _klick.metronome()->set_active(!active);
}


void TerminalHandler::update()
{
    MetronomeSimplePtr m = std::dynamic_pointer_cast<MetronomeSimple>(_klick.metronome());
    ASSERT(m);

    // clear current line
    std::cout << "\r\033[K\r";

    std::cout << "meter: " << m->beats() << "/" << m->denom() << ", ";
    std::cout << std::fixed << std::setprecision(0) << "tempo: " << m->tempo() << ", ";
    std::cout << std::setprecision(1) << "volume: " << _audio.volume();
    std::cout << std::flush;
}
