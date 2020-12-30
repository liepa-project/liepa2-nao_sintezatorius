/**
 * Žr. licenzijos failą LICENSE source medžio viršuje. (LICENSE file at the top of the source tree.)
 * 
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raštija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file main.cpp
 * 
 * @author dr. Margarita Beniušė (margarita.beniuse@mif.vu.lt), dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

#include <qi/applicationsession.hpp>
#include <boost/shared_ptr.hpp>
#include "liepaTTS.h"

int main(int argc, char* argv[])
{
    qi::ApplicationSession app(argc, argv);
    app.startSession();
    qi::SessionPtr session = app.session();
    session->registerService("LiepaTTS", qi::AnyObject(boost::make_shared<LiepaTTS>(session)));
    app.run();
    return 0;
}
