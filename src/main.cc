/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "main.hh"
#include "klick.hh"

#include <string>
#include <iostream>
#include <fstream>
#include <csignal>


static Klick *app = NULL;

static int ret = EXIT_SUCCESS;


std::string data_file(std::string const & path)
{
    std::fstream f;
    f.open(path.c_str(), std::ios::in);
    if (f.is_open()) {
        f.close();
        return path;
    }
    return std::string(DATA_DIR"/") + path;
}


static void signal_handler(int sig)
{
    app->signal_quit();

    if (sig != SIGINT) {
        ret = EXIT_FAILURE;
    }
}


int main(int argc, char *argv[])
{
    try {
        app = new Klick(argc, argv);

        // exit cleanly when terminated
        signal(SIGINT,  signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGHUP,  signal_handler);

        app->run();

        delete app;
        return ret;
    }
    catch (Exit const & e) {
        delete app;
        return e.status();
    }
    catch (std::exception const & e) {
        std::cerr << e.what() << std::endl;
        delete app;
        return EXIT_FAILURE;
    }
}
