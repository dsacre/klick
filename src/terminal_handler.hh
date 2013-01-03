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

#include <boost/noncopyable.hpp>

#include <termios.h>


class Klick;
class AudioInterface;


class TerminalHandler
  : boost::noncopyable
{
  public:
    TerminalHandler(Klick & klick, AudioInterface & audio);
    ~TerminalHandler();

    void handle_input();

  private:
    bool key_pressed();
    int get_key();
    int peek_key();
    bool munch_key(int);

    void set_beats(int);
    void set_denom(int);
    void change_tempo(float);
    void multiply_tempo(float);
    void change_volume(float);
    void toggle_running();

    void update();

    Klick & _klick;
    AudioInterface & _audio;

    ::termios _old_mode;
};
