/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "klick.h"

#include <string>
#include <iostream>
#include <csignal>


static Klick *app = NULL;


static void signal_handler(int sig)
{
    app->signal_quit();
}


static void terminate(const char *e)
{
    std::cerr << e << std::endl;
    delete app;
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    try {
        app = new Klick(argc, argv);

        // exit cleanly when terminated
        signal(SIGINT,  signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGQUIT, signal_handler);
        signal(SIGHUP,  signal_handler);

        app->run();
        delete app;

        return EXIT_SUCCESS;
    }
    catch (const char *e) {
        terminate(e);
    }
    catch (const std::string & e) {
        terminate(e.c_str());
    }
}
