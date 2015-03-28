/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2013  Dominic Sacré  <dominic.sacre@gmx.de>
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
#include <memory>
#include <csignal>


das::logstream logv(std::cout);


static std::shared_ptr<Klick> app;

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
    if (sig != SIGINT) {
        ret = EXIT_FAILURE;
    }

    app->signal_quit();
}


int main(int argc, char *argv[])
{
    try {
        app.reset(new Klick(argc, argv));

        // exit cleanly when terminated
        std::signal(SIGINT,  signal_handler);
        std::signal(SIGTERM, signal_handler);
        std::signal(SIGHUP,  signal_handler);

        app->run();

        return ret;
    }
    catch (Exit const & e) {
        return e.status();
    }
    catch (std::runtime_error const & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
