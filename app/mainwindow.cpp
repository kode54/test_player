#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QString>

#include <gme/gme.h>

#include <ao/ao.h>

#include <dumb.h>

extern "C" {
#include <internal/it.h>
}

#define SAMPLE_RATE 44100

class PlayerThread : public QThread
{
    volatile bool running;

    QString path;

public:
    PlayerThread(QObject * parent, QString const& _path)
        : QThread( parent ), path( _path )
    {
    }

    ~PlayerThread()
    {
        stop();
        wait();
    }

    void run()
    {
        Music_Emu * emu = NULL;

        ao_device * dev;
        ao_sample_format fmt = { 16, SAMPLE_RATE, 2, AO_FMT_NATIVE, 0 };

        signed short sample_buffer[2048 * 2];

        if ( !gme_open_file( path.toLocal8Bit().constData(), &emu, SAMPLE_RATE ) )
        {
            dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

            if ( dev )
            {
                gme_start_track( emu, 0 );

                running = true;
                while (running)
                {
                    gme_play( emu, 2048 * 2, sample_buffer );
                    ao_play( dev, (char *)sample_buffer, 2048 * 2 * sizeof(short) );
                }

                ao_close( dev );
            }

            gme_delete( emu );
        }
        else
        {
            DUH * duh = dumb_load_any_quick( path.toLocal8Bit().constData(), 0, 0 );
            if ( duh )
            {
                DUH_SIGRENDERER * sr = duh_start_sigrenderer( duh, 0, 2, 0 );
                if ( sr )
                {
                    DUMB_IT_SIGRENDERER * itsr = duh_get_it_sigrenderer( sr );
                    dumb_it_set_resampling_quality( itsr, 2 );
                    dumb_it_set_ramp_style( itsr, 2 );

                    sample_t ** buf = allocate_sample_buffer( 2, 2048 );
                    if ( buf )
                    {
                        dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                        if ( dev )
                        {
                            const float delta = 65536.0f / SAMPLE_RATE;

                            running = true;
                            while (running)
                            {
                                dumb_silence( buf[0], 2048 * 2 );
                                long written = duh_sigrenderer_generate_samples( sr, 1.0f, delta, 2048, buf );
                                for ( long i = 0; i < written; i++ )
                                {
                                    int sample = buf[0][ i * 2 + 0 ];
                                    if ( (unsigned)(sample + 0x800000) >= 0x1000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                    else sample >>= 8;
                                    sample_buffer[ i * 2 + 0 ] = sample;
                                    sample = buf[0][ i * 2 + 1 ];
                                    if ( (unsigned)(sample + 0x800000) >= 0x1000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                    else sample >>= 8;
                                    sample_buffer[ i * 2 + 1 ] = sample;
                                }
                                ao_play( dev, (char *)sample_buffer, written * 2 * sizeof(short) );
                                if ( written < 2048 ) break;
                            }

                            ao_close( dev );
                        }

                        destroy_sample_buffer( buf );
                    }

                    duh_end_sigrenderer( sr );
                }

                unload_duh( duh );
            }
        }
    }

    void stop()
    {
        running = false;
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    thread(0)
{
    dumb_it_max_to_mix = DUMB_IT_TOTAL_CHANNELS;
    dumb_register_stdfiles();

    ao_initialize();

    ui->setupUi(this);

    connect(ui->actionOpen,
            SIGNAL(triggered()),
            this,
            SLOT(appFileOpen()));

    connect(ui->actionExit,
            SIGNAL(triggered()),
            qApp,
            SLOT(quit()));
}

MainWindow::~MainWindow()
{
    delete thread;
    delete ui;

    ao_shutdown();
}

void MainWindow::appFileOpen()
{
    QString path = QFileDialog::getOpenFileName( this, "Select a song..." );

    if ( path.length() )
    {
        delete thread;

        thread = new PlayerThread( this, path );
        thread->start();
    }
}
