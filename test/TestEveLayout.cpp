#include <GG/EveLayout.h>

#include <GG/EveGlue.h>
#include <GG/EveParser.h>
#include <GG/GUI.h>
#include <GG/Timer.h>
#include <GG/Wnd.h>
#include <GG/SDL/SDLGUI.h>
#include <GG/adobe/adam.hpp>

#include <boost/filesystem.hpp>

#include <fstream>

#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include "TestingUtils.h"


const char* g_eve_file = 0;
const char* g_adam_file = 0;
const char* g_output_dir = 0;
bool g_dont_exit = false;

struct StopDialog
{
    StopDialog(GG::Wnd* dialog, const std::string& output) :
        m_dialog(dialog),
        m_output(output)
        {}
    void operator()(unsigned int, GG::Timer*)
        {
            GG::GUI::GetGUI()->SaveWndAsPNG(m_dialog, m_output);
            m_dialog->EndRun();
        }
    GG::Wnd* m_dialog;
    std::string m_output;
};

class MinimalGGApp : public GG::SDLGUI
{
public:
    MinimalGGApp();

    virtual void Enter2DMode();
    virtual void Exit2DMode();

private:
    virtual void GLInit();
    virtual void Initialize();
    virtual void FinalCleanup();
};

MinimalGGApp::MinimalGGApp() : 
    SDLGUI(1024, 768, false, "Minimal GG App")
{}

void MinimalGGApp::Enter2DMode()
{
    glPushAttrib(GL_ENABLE_BIT | GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, Value(AppWidth()), Value(AppHeight()));

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, Value(AppWidth()), Value(AppHeight()), 0.0, 0.0, Value(AppWidth()));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void MinimalGGApp::Exit2DMode()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void MinimalGGApp::GLInit()
{
    double ratio = Value(AppWidth() * 1.0) / Value(AppHeight());

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 0);
    glViewport(0, 0, Value(AppWidth()), Value(AppHeight()));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, ratio, 1.0, 10.0);
    gluLookAt(0.0, 0.0, 5.0,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
}

bool OkHandler(adobe::name_t name, const adobe::any_regular_t&)
{ return name == adobe::static_name_t("ok"); }

void MinimalGGApp::Initialize()
{
    std::ifstream eve(g_eve_file);
    std::ifstream adam(g_adam_file);
    GG::Wnd* eve_dialog = GG::MakeDialog(eve, adam, &OkHandler);

    boost::filesystem::path input(g_eve_file);
    boost::filesystem::path output(g_output_dir);
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION == 3
    output /= input.stem().native() + ".png";
#else
    output /= input.stem() + ".png";
#endif
    GG::Timer timer(100);
    if (!g_dont_exit)
        GG::Connect(timer.FiredSignal, StopDialog(eve_dialog, output.string()));
    eve_dialog->Run();

    Exit(0);
}

void MinimalGGApp::FinalCleanup()
{}

BOOST_AUTO_TEST_CASE( eve_layout )
{
    MinimalGGApp app;
    app();
}

// Most of this is boilerplate cut-and-pasted from Boost.Test.  We need to
// select which test(s) to do, so we can't use it here unmodified.

#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
bool init_unit_test()                   {
#else
::boost::unit_test::test_suite*
init_unit_test_suite( int, char* [] )   {
#endif

#ifdef BOOST_TEST_MODULE
    using namespace ::boost::unit_test;
    assign_op( framework::master_test_suite().p_name.value, BOOST_TEST_STRINGIZE( BOOST_TEST_MODULE ).trim( "\"" ), 0 );
    
#endif

#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
    return true;
}
#else
    return 0;
}
#endif

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
    g_eve_file = argv[1];
    g_adam_file = argv[2];
    g_output_dir = argv[3];
    if (argc == 5)
        g_dont_exit = true;
    return ::boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}
