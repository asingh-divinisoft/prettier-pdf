//
// Created by aaditya on 13/02/21.
//

#include "mainwindow.h"
#include "opencvhelper.h"
#include "images2pdf.h"
#include "qglobal.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QKeyEvent>
#include <QDebug>
#include <QProcess>
#include <QSpacerItem>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent)
    , fileMenu(nullptr)
    , viewMenu(nullptr)
    , currentImage(nullptr)
    , _convertor(nullptr)
{
    // initialize vars
    isPDF = false;
    isEdited = false;

    QString loc = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    dir = QDir(loc);
    if(!dir.exists(QString("WashDir"))){
        dir.mkdir(QString("WashDir"));
    }
    dir.cd(QString("WashDir"));

    initUI();
}

MainWindow::~MainWindow()
= default;

void MainWindow::initUI()
{
    this->resize(800, 600);
    // setup menubar
    fileMenu = menuBar()->addMenu("&File");
    viewMenu = menuBar()->addMenu("&View");

    // setup toolbar
    //fileToolBar = addToolBar("File");
    viewToolBar = addToolBar("View");
    addToolBarBreak(Qt::TopToolBarArea);
    editToolBar = addToolBar("Modify");


    // main area for image display
    imageScene = new QGraphicsScene(this);
    imageView = new QGraphicsView(imageScene);
    setCentralWidget(imageView);

    // setup status bar
    mainStatusBar = statusBar();
    mainStatusLabel = new QLabel(mainStatusBar);
    mainStatusBar->addPermanentWidget(mainStatusLabel);
    mainStatusLabel->setText("Image Information will be here!");

    createActions();
}

void MainWindow::createActions()
{

    // create actions, add them to menus
    openAction = new QAction("&Open Image", this);
    fileMenu->addAction(openAction);
    openPdfAction = new QAction("Open PDF", this);
    fileMenu->addAction(openPdfAction);
    saveAsAction = new QAction("Page -> PNG", this);
    saveAsAction->setToolTip("Save/Extract Current Page");
    fileMenu->addAction(saveAsAction);
    savePdfAction = new QAction("&Save PDF", this);
    fileMenu->addAction(savePdfAction);
    exitAction = new QAction("E&xit", this);
    fileMenu->addAction(exitAction);

    

    zoomInAction = new QAction("Zoom in", this);
    viewMenu->addAction(zoomInAction);
    zoomOutAction = new QAction("Zoom Out", this);
    viewMenu->addAction(zoomOutAction);
    prevAction = new QAction("&Previous Image", this);
    viewMenu->addAction(prevAction);
    nextAction = new QAction("&Next Image", this);
    viewMenu->addAction(nextAction);
    filterAction = new QAction("Filter", this);
    undoAction = new QAction("Undo", this);

    closeAction = new QAction("Close", this);

    shiftLeftAction = new QAction("Shift Left", this);
    shiftRightAction = new QAction("Shift Right", this);
    insertLeftAction = new QAction("Insert Left", this);
    insertRightAction = new QAction("Insert Right", this);

    // add actions to toolbars
    viewToolBar->addAction(openAction);
    viewToolBar->addAction(openPdfAction);

    auto* spacer1 = new QWidget(this);
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    viewToolBar->addWidget(spacer1);

    viewToolBar->addAction(shiftLeftAction);
    viewToolBar->addAction(shiftRightAction);
    viewToolBar->addAction(insertLeftAction);
    viewToolBar->addAction(insertRightAction);

    auto* spacer2 = new QWidget(this);
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    viewToolBar->addWidget(spacer2);

    viewToolBar->addAction(closeAction);
    viewToolBar->addAction(saveAsAction);
    viewToolBar->addAction(savePdfAction);

    auto* spacer3 = new QWidget(this);
    spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    editToolBar->addWidget(spacer3);

    editToolBar->addAction(zoomInAction);
    editToolBar->addAction(zoomOutAction);
    editToolBar->addAction(prevAction);
    editToolBar->addAction(nextAction);
    editToolBar->addAction(filterAction);
    editToolBar->addAction(undoAction);

    auto* spacer4 = new QWidget(this);
    spacer4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    editToolBar->addWidget(spacer4);

    toggleActions(false);

    // connect the signals and slots
    connect(exitAction, SIGNAL(triggered(bool)), QApplication::instance(), SLOT(quit()));
    connect(openAction, SIGNAL(triggered(bool)), this, SLOT(openImage()));
    connect(openPdfAction, SIGNAL(triggered(bool)), this, SLOT(openPdf()));
    connect(saveAsAction, SIGNAL(triggered(bool)), this, SLOT(saveImage()));
    connect(savePdfAction, SIGNAL(triggered(bool)), this, SLOT(savePdf()));
    connect(zoomInAction, SIGNAL(triggered(bool)), this, SLOT(zoomIn()));
    connect(zoomOutAction, SIGNAL(triggered(bool)), this, SLOT(zoomOut()));
    connect(prevAction, SIGNAL(triggered(bool)), this, SLOT(prevImage()));
    connect(nextAction, SIGNAL(triggered(bool)), this, SLOT(nextImage()));
    connect(filterAction, SIGNAL(triggered(bool)), this, SLOT(filterImage()));
    connect(undoAction, SIGNAL(triggered(bool)), this, SLOT(undoFilter()));
    connect(closeAction, SIGNAL(triggered(bool)), this, SLOT(closeThis()));
    connect(shiftLeftAction, SIGNAL(triggered(bool)), this, SLOT(shiftLeft()));
    connect(shiftRightAction, SIGNAL(triggered(bool)), this, SLOT(shiftRight()));
    connect(insertLeftAction, SIGNAL(triggered(bool)), this, SLOT(insertLeft()));
    connect(insertRightAction, SIGNAL(triggered(bool)), this, SLOT(insertRight()));

    setupShortcuts();
}

void MainWindow::toggleActions(bool state){
    prevAction->setEnabled(state);
    nextAction->setEnabled(state);
    zoomInAction->setEnabled(state);
    zoomOutAction->setEnabled(state);
    filterAction->setEnabled(state);
    undoAction->setEnabled(state);
    closeAction->setEnabled(state);
    saveAsAction->setEnabled(state);
    savePdfAction->setEnabled(state);
    shiftLeftAction->setEnabled(state);
    shiftRightAction->setEnabled(state);
    insertLeftAction->setEnabled(state);
    insertRightAction->setEnabled(state);
}

void MainWindow::closeThis() {
    isPDF = false;
    isOpen = false;
    isEdited = false;
    lastImageAvailable = false;
    current_pg = 0;
    mainStatusLabel->setText(QString());
    currentImagePath = QString();
    history.clear();
    order.clear();
    delete _convertor;
    _convertor = nullptr;
    lastImage = QImage();
    currentImage->setPixmap(QPixmap());
    toggleActions(false);
}

void MainWindow::openImage()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.png *.bmp *.jpg)"));
    QStringList filePaths;
    if (dialog.exec()) {
        filePaths = dialog.selectedFiles();
        isPDF = false;

        toggleActions(true);
        nextAction->setEnabled(false);
        prevAction->setEnabled(false);

        showImage(filePaths.at(0));
    }
}

void MainWindow::openPdf()
{
    // TODO: show page no.
    // TODO: add filter all buttons : progress bar
    // TODO: fix build warnings
    // TODO: set default scale to something
    // TODO: maybe add scrolling view like word 

    for (const QString& dirFile : dir.entryList()) {
        dir.remove(dirFile);
    }

    QFileDialog dialog(this);
    dialog.setWindowTitle("Open PDF");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("PDFs (*.pdf)"));
    QStringList filePaths;

    if (dialog.exec()) {
        filePaths = dialog.selectedFiles();
        QProcess process;
        process.setWorkingDirectory(QCoreApplication::applicationDirPath());

        QString basepath = dir.absolutePath() + "/pg";
        #ifdef Q_OS_LINUX
            process.start("pdftopng", QStringList() << filePaths.at(0) << basepath);
        #else
            process.start("cmd", QStringList() << "/c" << "pdftopng.exe" << filePaths.at(0) << basepath);
        #endif

        if (!process.waitForFinished()) {
            QMessageBox::information(this, "Information", "Could not convert to PNG.");
            return;
        }

        isPDF = true;

        toggleActions(true);

        dir.refresh();
        QStringList nameFilters;
        nameFilters << "*.png";
        auto filenames = dir.entryList(nameFilters);

        for(const QString& file : filenames){
            order.push_back(file);
        }
        current_pg = 0;

        showImage(QString(basepath+"-000001.png"));
    }
}

void MainWindow::showImage(const QString& path)
{
    isEdited = false;
    history.clear();

    lastImageAvailable = false;
    imageScene->clear();
    imageView->resetMatrix();
    QPixmap image(path);
    currentImage = imageScene->addPixmap(image);
    imageScene->update();
    imageView->setSceneRect(image.rect());
    QString status = QString("%1, %2x%3, %4 Bytes").arg(path).arg(image.width())
            .arg(image.height()).arg(QFile(path).size());
    mainStatusLabel->setText(status);
    currentImagePath = path;
}

void MainWindow::zoomIn()
{
    imageView->scale(1.2, 1.2);
}

void MainWindow::zoomOut()
{
    imageView->scale(1/1.2, 1/1.2);
}

void MainWindow::prevImage()
{
    if (isPDF && isEdited) {
        currentImage->pixmap().save(currentImagePath, nullptr, 0);
    }
    if(current_pg > 0) {
        showImage(dir.absoluteFilePath(order[--current_pg]));
    } else {
        QMessageBox::information(this, "Information", "Current image is the first one.");
    }
}

void MainWindow::nextImage()
{
    if (isPDF && isEdited) {
        currentImage->pixmap().save(currentImagePath, nullptr, 0);
    }
    if(current_pg < order.size() - 1) {
        showImage(dir.absoluteFilePath(order[++current_pg]));
    } else {
        QMessageBox::information(this, "Information", "Current image is the last one.");
    }
}

void MainWindow::shiftLeft(){
    if(current_pg > 0){
        std::swap(order[current_pg], order[current_pg-1]);
        current_pg--;
    }
    for(auto& x: order){
        std::cout << x.toStdString() << "\n";
    }
}

void MainWindow::shiftRight(){
    if(current_pg < order.size()-1){
        std::swap(order[current_pg], order[current_pg+1]);
        current_pg++;
    }
    for(auto& x: order){
        std::cout << x.toStdString() << "\n";
    }
}

void MainWindow::insertLeft(){
    char num_s[7];
    snprintf(num_s, 7, "%06d", static_cast<int>(order.size()) + 1);

    QString newName = QString("pg-%1.png").arg(num_s);

    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.png *.bmp *.jpg)"));
    QStringList filePaths;
    if (dialog.exec()) {
        order.insert(order.begin()+current_pg, newName);

        filePaths = dialog.selectedFiles();
//        QString name = QFile(filePaths.at(0)).fileName();
        QString newPath = QString("%1/%2").arg(dir.absolutePath(), newName);
        QFile::copy(filePaths.at(0), newPath);

        showImage(newPath);
    }
}

void MainWindow::insertRight(){
    char num_s[7];
    QString newName;
    snprintf(num_s, 7, "%06d", static_cast<int>(order.size()) + 1);

    newName = QString("pg-%1.png").arg(num_s);

    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.png *.bmp *.jpg)"));
    QStringList filePaths;
    if (dialog.exec()) {
        current_pg++;
        order.insert(order.begin()+current_pg, newName);

        filePaths = dialog.selectedFiles();
//        QString name = QFile(filePaths.at(0)).fileName();
        QString newPath = QString("%1/%2").arg(dir.absolutePath(), newName);
        QFile::copy(filePaths.at(0), newPath);

        showImage(newPath);
    }
}

void MainWindow::saveImage()
{
    if (currentImage == nullptr) {
        QMessageBox::information(this, "Information", "Nothing to save.");
        return;
    }
    QFileDialog dialog(this);
    dialog.setWindowTitle("Save Image As ...");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("Images (*.png *.bmp *.jpg)"));
    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
        if(QRegExp(".+\\.(png|bmp|jpg)").exactMatch(fileNames.at(0))) {
            currentImage->pixmap().save(fileNames.at(0));
        } else {
            QMessageBox::information(this, "Information", "Save error: bad format or filename.");
        }
    }
}



void MainWindow::savePdf() {
    if (currentImage == nullptr) {
        QMessageBox::information(this, "Information", "Nothing to save.");
        return;
    }
    QFileDialog dialog(this);
    dialog.setWindowTitle("Save PDF As ...");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("PDFs (*.pdf)"));
    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
        if (QRegExp(".+\\.(pdf)").exactMatch(fileNames.at(0))) {
            QStringList l;
            for(const QString& x : order){
                l << x;
            }
            _convertor = new Images2PDF(dir.absolutePath(), std::move(l), fileNames.at(0));
            connect(_convertor, SIGNAL(finished(bool)), this, SLOT(processFinished(bool)));
            _convertor->start();
        }
        else {
            QMessageBox::information(this, "Information", "Save error: bad format or filename.");
        }
    }
}

void MainWindow::processFinished(bool success){
    if (success) {
        QMessageBox::information(this, "Information", "Succesfully saved pdf.");
    }
    else if (_convertor) {
        QMessageBox::information(this, "Information", "Error in saving pdf.");
    }
    if (_convertor) {
        delete _convertor;
        _convertor = nullptr;
    }
}

void MainWindow::filterImage() {
    if (currentImage == nullptr) {
        QMessageBox::information(this, "Information", "Nothing to filter.");
        return;
    }
    history.emplace_back(&documentFilter);
    applyFilters();
}

void MainWindow::applyFilters() {
    lastImage = currentImage->pixmap().toImage();
    lastImageAvailable = true;
    isEdited = true;
    currentImage->setPixmap(QPixmap::fromImage(history.back()(lastImage)));
}

void MainWindow::undoFilter(){
    if(history.empty()){ return; }
    history.pop_back();
    if(history.empty()){
        currentImage->setPixmap(QPixmap(currentImagePath));
        lastImage = QImage();
        lastImageAvailable = false;
        isEdited = false;
        return;
    }
    if(lastImageAvailable){
        currentImage->setPixmap(QPixmap::fromImage(lastImage));
        lastImage = QImage();
        lastImageAvailable = false;
        return;
    }
    
    QImage tempImage = QImage(currentImagePath);
    for(auto it = history.begin(); it != history.end()-1; it++){
        tempImage = (*it)(tempImage);
    }
    lastImage = tempImage;
    lastImageAvailable = true;
    tempImage = (*(history.end()-1))(tempImage);
    currentImage->setPixmap(QPixmap::fromImage(tempImage));
}

void MainWindow::setupShortcuts()
{
    QList<QKeySequence> shortcuts;
    shortcuts << Qt::Key_Plus << Qt::Key_Equal;
    zoomInAction->setShortcuts(shortcuts);

    shortcuts.clear();
    shortcuts << Qt::Key_Minus << Qt::Key_Underscore;
    zoomOutAction->setShortcuts(shortcuts);

    shortcuts.clear();
    shortcuts << Qt::Key_Up << Qt::Key_Left;
    prevAction->setShortcuts(shortcuts);

    shortcuts.clear();
    shortcuts << Qt::Key_Down << Qt::Key_Right;
    nextAction->setShortcuts(shortcuts);
}
