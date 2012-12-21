#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QString>

#include <gme/gme.h>

#include <ao/ao.h>

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

        if ( !gme_open_file( path.toLocal8Bit().constData(), &emu, SAMPLE_RATE ) )
        {
            ao_device * dev;
            ao_sample_format fmt = { 16, SAMPLE_RATE, 2, AO_FMT_NATIVE, 0 };

            signed short sample_buffer[2048 * 2];

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
