/****************************************************************************
 * EZ Viewer
 * Copyright (C) 2012 huangezhao. CHINA.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ***************************************************************************/

#include "mainwindow.h"

#include "settingwidget.h"
#include "global.h"
#include <QtGui>

#include <QtDebug>
#include "floatframe.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    WasMaximized = false;
    slideInterval = 4000;
    slideTimer = new QTimer(this);
    slideTimer->setInterval(slideInterval);
    connect(slideTimer, SIGNAL(timeout()), SLOT(nextPic()));

    viewer = new ImageViewer(this);
    setCentralWidget(viewer);

//    statusBar()->showMessage(tr("Status Bar"));

//!    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint); //!
    //Qt::FramelessWindowHint | Qt::WindowTitleHint);
//    setMinimumSize(150, 100);

    connect(viewer, SIGNAL(fileNameChange(QString)), SLOT(setMyWindowTitle(QString)));
    connect(viewer, SIGNAL(mouseDoubleClick()), SLOT(changeFullScreen()));
    connect(viewer, SIGNAL(showContextMenu(QPoint)), SLOT(showContextMenu(QPoint)));
    connect(viewer, SIGNAL(needOpenFile(QStringList)), SLOT(openFile(QStringList))); ///
    connect(viewer, SIGNAL(siteChange(QPoint)), SLOT(moveWindow(QPoint)));
    connect(viewer, SIGNAL(sizeChange(QSize)), SLOT(resizeWindow(QSize)));

    initContextMenu();
    setMyWindowTitle();
    setWindowIcon(QIcon(":/res/twitter.png"));
    setAttribute(Qt::WA_DeleteOnClose); //! !!!

    initTitleButton();
}

void MainWindow::initTitleButton() //! QAction????
{
    titleButtonFF = new FloatFrame(this);
    titleButtonFF->resize(120, 40);
    titleButtonFF->move(width() - 120, 0);
    connect(titleButtonFF, SIGNAL(showContextMenu(QPoint)),
            SLOT(showContextMenu(QPoint)));

    QStyle *st = qApp->style();

    QPushButton *minButton = new QPushButton(titleButtonFF);
    minButton->setIcon(st->standardIcon(QStyle::SP_TitleBarMinButton));
    minButton->setFlat(true);
    minButton->setToolTip(tr("Minimize"));
    connect(minButton, SIGNAL(clicked()), SLOT(showMinimized()));

    QPushButton *closeButton = new QPushButton(titleButtonFF);
    closeButton->setIcon(st->standardIcon(QStyle::SP_DialogCloseButton));
    closeButton->setFlat(true);
    closeButton->setToolTip(tr("Close"));
    connect(closeButton, SIGNAL(clicked()), SLOT(close()));

    QHBoxLayout *hlayout = new QHBoxLayout(titleButtonFF);
    hlayout->addWidget(minButton);
    hlayout->addWidget(closeButton);
    titleButtonFF->setLayout(hlayout);

    titleButtonFF->addWidget(minButton);
    titleButtonFF->addWidget(closeButton);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    titleButtonFF->move(width() - 120, 0);
    QWidget::resizeEvent(e);
}

void MainWindow::about()
{
    QMessageBox::about(this,
                       tr("About %1").arg(GlobalStr::PROJECT_NAME()),
                       GlobalStr::ABOUT_TEXT());
}

void MainWindow::setMyWindowTitle(const QString &title)
{
    if(title.isEmpty())
        setWindowTitle(GlobalStr::PROJECT_NAME());
    else
        setWindowTitle(title);
}

void MainWindow::openFile()
{
    QString currentFile(viewer->currentFile());
    QString defaultDir(currentFile.isEmpty()
                       ? QDesktopServices::storageLocation(QDesktopServices::PicturesLocation)
                       : QFileInfo(currentFile).absolutePath());
    QString fileName =
            QFileDialog::getOpenFileName(
                this, tr("Open File"), defaultDir,
                tr("Images (%1);;All Files (*)").arg(SUPPORT_FORMAT));
    if (!fileName.isEmpty())
        viewer->openFile(fileName);
}

MainWindow *MainWindow::creatMainWindow(int sizeMode, int antialiasMode,
            bool enableBgColor, const QColor &bgColor, int timerInterval)
{
    MainWindow *window = new MainWindow;
    window->changeAntialiasMode(antialiasMode);
    if(enableBgColor)
        window->changeBgColor(bgColor);
    window->changeTimerInterval(timerInterval);
    window->enableSelfAdaptive(sizeMode == 3);
    switch(sizeMode){
    case 2:
        window->showFullScreen();
        break;
    case 1:
        window->showMaximized();
        break;
//    case 3:
//    case 0:
    default:
        window->resize(FIT_SIZE);
        window->show();
        break;
    }

    return window;
}

void MainWindow::openFile(const QStringList &list) //! static method
{
    QSettings *settings = new QSettings(INI_FILE_PATH, QSettings::IniFormat);
    int sizeMode = settings->value(SizeModeKey, 0).toInt();
    bool showDialog = settings->value(DialogKey, true).toBool();
    int antialiasMode = settings->value(AntialiasModeKey, 0).toInt();
    bool enableBgColor = settings->value(EnableBgColorKey, true).toBool();
    QString colorStr = settings->value(BgColorKey, BG_GREEN).toString();
    int timerInterval = settings->value(TimerIntervalKey, 4).toInt();

    if(sizeMode < 0 || sizeMode > 3)// 4 modes
        sizeMode = 0;
    if(antialiasMode < 0 || antialiasMode > 2) // 3 modes
        antialiasMode = 0;
    QColor bgColor(colorStr);
    if(!bgColor.isValid())
        bgColor.setNamedColor(BG_GREEN);
    if(timerInterval < 1 || timerInterval > 1000)
        timerInterval = 4;

    QStringList fileList;
    QFileInfo fileInfo;
    QString fileName;
    for (int size = list.size(), i = 0; i < size; ++i) {
        fileName = list.at(i);
        fileInfo.setFile(fileName);
        if(fileInfo.isFile())//no directory
            fileList.append(fileName);
    }

    MainWindow *window;
    if(fileList.empty()){
        if(qApp->topLevelWidgets().empty()){
            window = creatMainWindow(sizeMode, antialiasMode,
                                  enableBgColor, bgColor, timerInterval);
            if(showDialog)
                window->openFile();
        }
    }else{  //! is multi-threads needs？？
        for(int size = fileList.size(), i=0; i < size; ++i){
            window = creatMainWindow(sizeMode, antialiasMode,
                                  enableBgColor, bgColor, timerInterval);
            window->openFile(fileList.at(i));
        }
    }
}

void MainWindow::changeFullScreen()
{
//    setWindowState( windowState() ^ Qt::WindowFullScreen );
    if(isFullScreen()){
//        statusBar()->show();
        if(WasMaximized)
            showMaximized();
        else
            showNormal();
    }else{
        WasMaximized = isMaximized();    //! only for windows?
//        statusBar()->hide();
        showFullScreen();
    }
}

void MainWindow::showAttribute()
{
    if(viewer->hasPicture() || viewer->hasFile()){
#ifdef Q_WS_WIN
        QWhatsThis::showText(QCursor::pos(), viewer->attributeString(), this);
#else//(Q_WS_X11)
        QMessageBox::information(this, tr("Property"), viewer->attributeString());
#endif
        //    setToolTip(viewer->attributeString());
        //    QToolTip::showText(QCursor::pos(), viewer->attributeString(), this);
    }
}

void MainWindow::slideShow()// if other commend when slide show??
{
    if(!viewer->hasFile())
        return;

    if(slideTimer->isActive()){
        slideTimer->stop();
        slideAction->setIcon(QIcon(":/res/Play.png"));
        slideAction->setText(tr("Auto Play"));
    }else{
        slideTimer->start();
        slideAction->setIcon(QIcon(":/res/Stop.png"));
        slideAction->setText(tr("Stop Play"));
    }
}

void MainWindow::initContextMenu()
{
    QStyle *st = qApp->style();

    QAction *settingAction = new QAction(QIcon(":/res/Setting.png"),
                                         tr("&Setting"), this);
    connect(settingAction, SIGNAL(triggered()), SLOT(setting()));

    QAction *aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, SIGNAL(triggered()), SLOT(about()));

    QAction *closeAction = new QAction(tr("Close"), this);
    connect(closeAction, SIGNAL(triggered()), SLOT(close()));

    openAction = new QAction(
                st->standardIcon(QStyle::SP_DialogOpenButton),
                tr("&Open"), this);
    connect(openAction, SIGNAL(triggered()), SLOT(openFile()));

    slideAction = new QAction(QIcon(":/res/Play.png"),
                              tr("Auto Play"), this);
    connect(slideAction, SIGNAL(triggered()), SLOT(slideShow()));

    rotateLeftAction = new QAction(QIcon(":/res/Undo.png"),
                                   tr("Rotate &Left"), this);
    connect(rotateLeftAction, SIGNAL(triggered()), SLOT(rotateLeft()));

    rotateRightAction = new QAction(QIcon(":/res/Redo.png"),
                                    tr("Rotate &Right"), this);
    connect(rotateRightAction, SIGNAL(triggered()), SLOT(rotateRight()));

    mirrorHAction = new QAction(tr("Mirrored &Horizontal"), this);
    connect(mirrorHAction, SIGNAL(triggered()), SLOT(mirrorHorizontal()));

    mirrorVAction = new QAction(tr("Mirrored &Vertical"), this);
    connect(mirrorVAction, SIGNAL(triggered()), SLOT(mirrorVertical()));

    copyAction = new QAction(tr("&Copy to clipboard"), this);
    connect(copyAction, SIGNAL(triggered()), SLOT(copyToClipboard()));

    attributeAction = new QAction(QIcon(":/res/Info.png"),
                                  tr("&Property"), this);
    connect(attributeAction, SIGNAL(triggered()), SLOT(showAttribute()));

    deleteAction = new QAction(
                st->standardIcon(QStyle::SP_DialogCloseButton),
                tr("&Delete"), this);
    connect(deleteAction, SIGNAL(triggered()), SLOT(deleteFileAsk()));

    exitAct = new QAction(tr("Close All"), this);
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    contextMenu = new QMenu(this);
    contextMenu->addAction(openAction);
    contextMenu->addAction(slideAction);
    contextMenu->addSeparator();
    contextMenu->addAction(rotateLeftAction);
    contextMenu->addAction(rotateRightAction);
    contextMenu->addAction(mirrorHAction);
    contextMenu->addAction(mirrorVAction);
    contextMenu->addSeparator();
    contextMenu->addAction(copyAction);
    contextMenu->addAction(deleteAction);
    contextMenu->addAction(attributeAction);
    contextMenu->addSeparator();
    contextMenu->addAction(settingAction);
    contextMenu->addAction(aboutAction);
    contextMenu->addSeparator();
    contextMenu->addAction(closeAction);
    contextMenu->addAction(exitAct);
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    bool hasPixmap = viewer->hasPicture();
    bool has_file = viewer->hasFile();
    //! QMenu is top-level window, no matter hidden or shown.
    bool multiWindow = (qApp->topLevelWidgets().size() > 2);
    bool notSliding = (!slideTimer->isActive());

    openAction->setEnabled(notSliding);
    slideAction->setEnabled(has_file);
    rotateLeftAction->setEnabled(hasPixmap);
    rotateRightAction->setEnabled(hasPixmap);
    mirrorHAction->setEnabled(hasPixmap);
    mirrorVAction->setEnabled(hasPixmap);
    copyAction->setEnabled(hasPixmap);
    deleteAction->setEnabled(has_file && notSliding);
    attributeAction->setEnabled(hasPixmap || has_file); //
    exitAct->setVisible(multiWindow);

    contextMenu->popup(pos);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
//    const int CommandCount = 18;
//    QStringList CommandList;
//    CommandList << "openFile" << "changeFullScreen" << "slideShow"
//                << "showAttribute" << "setting" << "about"
//                << "nextPic" << "prePic" << "closeWindow"
//                << "rotateLeft" << "rotateRight" << "mirrorHorizontal"
//                << "mirrorVertical" << "switchGifPause" << "nextGifFrame"
//                << "copyToClipboard" << "deleteFile" << "closeAllWindows";

//    void (MainWindow::*Func[CommandCount])() = {   //函数指针数组
//                &MainWindow::openFile,       &MainWindow::changeFullScreen,
//                &MainWindow::slideShow,      &MainWindow::showAttribute,
//                &MainWindow::setting,        &MainWindow::about,
//                &MainWindow::nextPic,        &MainWindow::prePic,
//                &MainWindow::closeWindow,          &MainWindow::rotateLeft,
//                &MainWindow::rotateRight,    &MainWindow::mirrorHorizontal,
//                &MainWindow::mirrorVertical, &MainWindow::switchGifPause,
//                &MainWindow::nextGifFrame,   &MainWindow::copyToClipboard,
//                &MainWindow::deleteFile,     &MainWindow::closeAllWindows
//    };

    //(this->*Func[i])(end);  //执行相应指令对应的函数


//    qDebug() << "key code : " << QString::number(e->key(), 16);
//    qDebug() << "key text: " << e->text();
//    if(e->modifiers() & Qt::NoModifier)
//        qDebug() << "Qt::NoModifier";
//    if(e->modifiers() & Qt::ShiftModifier)
//        qDebug() << "Qt::ShiftModifier";
//    if(e->modifiers() & Qt::ControlModifier)
//        qDebug() << "Qt::ControlModifier";
//    if(e->modifiers() & Qt::AltModifier)
//        qDebug() << "Qt::AltModifier";
//    if(e->modifiers() & Qt::MetaModifier)
//        qDebug() << "Qt::MetaModifier";

    switch(e->key()){
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_PageDown:
    case Qt::Key_J:
        nextPic();
        e->accept();
        break;
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_PageUp:
    case Qt::Key_K:
        prePic();
        e->accept();
        break;
    case Qt::Key_O:
    case Qt::Key_N:
        if(!slideTimer->isActive())
            openFile();
        e->accept();
        break;
    case Qt::Key_Escape:
    case Qt::Key_Q:
        close();        //! !
        e->accept();
        break;
    case Qt::Key_Equal:
    case Qt::Key_Plus:
        switch(e->modifiers()){
        case Qt::ShiftModifier:
            viewer->zoomIn(0.05);
            break;
        case Qt::ControlModifier:
            viewer->zoomIn(0.2);
            break;
        default:
            viewer->zoomIn(0.1);
            break;
        }
        e->accept();
        break;
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        switch(e->modifiers()){
        case Qt::ShiftModifier:
            viewer->zoomIn(-0.05);
            break;
        case Qt::ControlModifier:
            viewer->zoomIn(-0.2);
            break;
        default:
            viewer->zoomIn(-0.1);
            break;
        }
        e->accept();
        break;
    case Qt::Key_Return:
        changeFullScreen();
        e->accept();
        break;
    case Qt::Key_L:
        rotateLeft();
        e->accept();
        break;
    case Qt::Key_R:
        rotateRight();
        e->accept();
        break;
    case Qt::Key_H:
        mirrorHorizontal();
        e->accept();
        break;
    case Qt::Key_V:
        mirrorVertical();
        e->accept();
        break;
    case Qt::Key_I:
        showAttribute();
        e->accept();
        break;
    case Qt::Key_Delete:
    case Qt::Key_D:
        if(slideTimer->isActive())
            break;
        if(e->modifiers() == Qt::ControlModifier)
            deleteFileNoAsk();
        else
            deleteFileAsk();
        e->accept();
        break;
    case Qt::Key_C:
        if(e->modifiers() == Qt::ControlModifier){
            copyToClipboard();
            e->accept();
        }
        break;
    case Qt::Key_S:
        setting();
        e->accept();
        break;
    case Qt::Key_Space:
    case Qt::Key_Pause:
        switchGifPause();
        e->accept();
        break;
    case Qt::Key_F:
        nextGifFrame();
        e->accept();
        break;
    case Qt::Key_P:
        slideShow();
        e->accept();
        break;
    default:
        QWidget::keyPressEvent(e);
        break;
    }
}

void MainWindow::setting()
{
    QDialog *dlg = new QDialog(this, Qt::MSWindowsFixedSizeDialogHint
                               | Qt::WindowTitleHint);
    dlg->setWindowTitle(GlobalStr::PROJECT_NAME());
    SettingWidget *settingWidget = new SettingWidget(dlg);
    connect(settingWidget, SIGNAL(clickClose()), dlg, SLOT(close()));
    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->addWidget(settingWidget);
    dlg->setLayout(layout);

    MainWindow *mainWindow;
    foreach(QWidget *window, qApp->topLevelWidgets()){
//        if(QString("MainWindow") == window->metaObject()->className()){
            mainWindow = dynamic_cast<MainWindow*>(window);
            if(mainWindow){
                connect(settingWidget, SIGNAL(changeAntialiasMode(int)),
                        mainWindow, SLOT(changeAntialiasMode(int)));
                connect(settingWidget, SIGNAL(changeBgColor(QColor)),
                        mainWindow, SLOT(changeBgColor(QColor)));
                connect(settingWidget, SIGNAL(enableSelfAdaptive(bool)),
                        mainWindow, SLOT(enableSelfAdaptive(bool)));
                connect(settingWidget, SIGNAL(changeTimerInterval(int)),
                        mainWindow, SLOT(changeTimerInterval(int)));
            }
//        }
    }

    dlg->exec();
    delete dlg;
}
