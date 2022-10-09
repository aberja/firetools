/*
 * Copyright (C) 2015-2018 Firetools Authors
 *
 * This file is part of firetools project
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "firetools.h"
#include <QtGlobal>
#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../../firetools_config_extras.h"
#include "../../firetools_config.h"
#include "mainwindow.h"
#include "../common/utils.h"
#include "applications.h"

MainWindow::MainWindow(QWidget *parent): QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint) {
	active_index_ = -1;
	edit_index_ = -1;
	animation_id_ = 0;
//	stats_ = new StatsDialog();
//	connect(this, SIGNAL(cycleReadySignal()), stats_, SLOT(cycleReady()));

	// check firejail
	if (!which("firejail")) {
		QMessageBox::warning(this, tr("Firejail Launcher"),
			tr("<br/><b>Firejail</b> software not found. Please install it.<br/><br/><br/>"));
		exit(1);
	}

	// check if we have permission to run firejail
	char *testrun = run_program("firejail /bin/true 2>&1");
	if (!testrun || strstr(testrun, "Error")) {
		QMessageBox::warning(this, tr("Firejail Launcher"),
			tr("<br/>Cannot run <b>Firejail</b> sandbox, you may not have<br/>the correct permissions to access this program.<br/><br/><br/>"));
		exit(1);
	}

	// check svg support
#if QT_VERSION >= 0x050000
	QList<QByteArray> flist = QImageReader::supportedImageFormats();
	bool svgfound = false;
	for (int i = 0; i < flist.size(); i++) {
		QByteArray a = flist.at(i);
		const char *str = a.constData();
		if (strcmp(str, "svg") == 0)
			svgfound = true;
	}
	if (!svgfound) {
		QMessageBox::warning(this, tr("Firejail Launcher"),
			tr("<br/>Qt5 SVG icon library not found. Please install it:<br/>"
			    "sudo apt-get install libqt5svg5<br/><br/>"));
	}
#endif

	applications_init();
	createTrayActions();
	createLocalActions();
//	thread_ = new PidThread();
//	connect(thread_, SIGNAL(cycleReady()), this, SLOT(cycleReady()));

	setContextMenuPolicy(Qt::ActionsContextMenu);
	setToolTip(tr("Double click on an icon to open an application.\n"
		"Drag the launcher with the left mouse button.\n"
		"Use the right mouse button to open a context menu."));
	setWindowTitle(tr("Firetools"));

}

void MainWindow::edit() {
#if 0
	if (edit_index_ != -1) {
		EditDialog *edit;

		// new entry
		if (active_index_ == -1) {
			edit = new EditDialog("", "", "");
			if (QDialog::Accepted == edit->exec()) {
				// check if the sandbox already exists
				QString name = edit->getName();
				if (applist_check(name) == false && applications_check_default(name.toLocal8Bit().constData()) == false) {
					Application app(edit->getName(), edit->getDescription(), edit->getCommand(), edit->getName());
					app.saveConfig();
					applist.append(app);
					if (arg_debug) {
						printf("Application added:\n");
						applist_print();
					}
				}
				else
					QMessageBox::critical(this, tr("Firejail Tools"),
						tr("<br/>Sandbox already defined.<br/><br/><br/>"));
			}
		}

		// existing entry
		else {
printf("%s\n", applist[active_index_].exec_.toLocal8Bit().constData());
			edit = new EditDialog(applist[active_index_].name_, applist[active_index_].description_, applist[active_index_].exec_);
			if (QDialog::Accepted == edit->exec()) {
				applist[active_index_].name_ = edit->getName();
				applist[active_index_].description_ = edit->getDescription();
				applist[active_index_].exec_ = edit->getCommand();
				applist[active_index_].saveConfig();
			}
		}
		delete edit;

		// update
		hide();
		show();
		update();
	}
#endif
}

// Remove application from the list
void MainWindow::remove() {
//printf("line %d, active index %d, name %s\n", __LINE__, active_index_,
//	applist[active_index_].name_.toLocal8Bit().constData());

	char *fname = get_config_file_name(applist[active_index_].name_.toLocal8Bit().constData());
	if (fname) {
		unlink(fname);
		applist.removeAt(active_index_);
		if (arg_debug) {
			printf("Application removed:\n");
			applist_print();
		}

		// update
		hide();
		show();
		update();
	}
	free(fname);
}


// Run application
void MainWindow::run() {
	int index = active_index_;
	if (index != -1) {
		QString exec = applist[index].exec_ + " &";
		int rv = system(exec.toStdString().c_str());
		(void) rv;
	}

	animation_id_ = AFRAMES;
	QTimer::singleShot(0, this, SLOT(update()));
}

// Run statistics tools
void MainWindow::runTools() {
	// start fstats as a separate process
	int rv = system(PACKAGE_LIBDIR "/fstats &");
	(void) rv;
}

// Start firejail-ui
void MainWindow::newSandbox() {
	// start firejail-ui as a separate process
	int rv = system("firejail-ui &");
	(void) rv;
}

// About window
void MainWindow::runAbout() {
	QString msg = "<table cellpadding=\"10\"><tr><td><img src=\":/resources/firetools.png\"></td>";
	msg += "<td>" + tr(

		"Firetools is a GUI application for Firejail. "
		"It offers a system tray launcher for sandboxed apps, "
		"sandbox editing, management, and statistics. "
		"The software package also includes a sandbox configuration wizard, firejail-ui.<br/><br/>"

		"Firejail  is  a  SUID sandbox program that reduces the risk of security "
		"breaches by restricting the running environment of  untrusted  applications "
		"using Linux namespaces, Linux capabilities and seccomp-bpf.<br/><br/>") +
		tr("Firetools version:") + " " + PACKAGE_VERSION + "<br/>" +
		tr("QT version: ") + " " + QT_VERSION_STR + "<br/>" +
		tr("License:") + " GPL v2<br/>" +
		tr("Homepage:") + " " + QString(PACKAGE_URL) + "</td></tr></table><br/><br/>";

	QMessageBox::about(this, tr("About"), msg);
}

// Mouse events: mouse release
void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
	int nelem = applist.count();
	int cols = nelem / ROWS + 1;

	if (event->button() == Qt::LeftButton) {
		int x = event->pos().x();
		int y = event->pos().y();

		if (x >= MARGIN * 2 + cols * 64 - 16 && x <= MARGIN * 2 + cols * 64 + 4 &&
			   y >= 4 && y <= 15) {

			showMinimized();
		}
		event->accept();
		active_index_ = -1;
	}
}

// Mouse events: mouse press
void MainWindow::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		dragPosition_ = event->globalPos() - frameGeometry().topLeft();
		event->accept();
		active_index_ = -1;
	}

	else if (event->button() == Qt::RightButton) {
	}
}


// Mouse events
void MainWindow::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() & Qt::LeftButton) {
		move(event->globalPos() - dragPosition_);
		event->accept();
	}
}

// Mouse events: double-click
void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		QPoint pos = event->pos();
		int index = applications_get_index(pos);
		if (index != -1) {
			QString exec = applist[index].exec_ + " &";
			int rv = system(exec.toStdString().c_str());
			(void) rv;
			event->accept();
			animation_id_ = AFRAMES;
			active_index_ = index;
			QTimer::singleShot(0, this, SLOT(update()));
		}
	}
}

// Main window visual design
void MainWindow::paintEvent(QPaintEvent *) {
	// Count the number applications and put the value to the variable
	int nelem = applist.count();

	// Columns is the amount of applications divided by number of rows + 1
	int cols = nelem / ROWS + 1;

	// Start painting
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// Window size hint
	QSize sz = sizeHint();

	// Window rectangle size coordinates
	QRect windowRectSize(0, 0, sz.width(), sz.height());

	// Background color for the main window
	// (dark gray)
	QBrush windowBackgroundColor(QColor(68, 68, 68));

	// Fills the given rectangle with the specified color values.
	// https://doc.qt.io/qt-5.10/qpainter.html#drawRect
	painter.fillRect(windowRectSize, windowBackgroundColor);

	// Loop icons to rows
	int i = 0;
	int j = 0;
	for (; i < nelem; i++, j++) {
		if (j >= ROWS)
			j = 0;

		// Select icon from the looped items
		QIcon icon = applist[i].app_icon_;

		int sz = 64 ;
		if (active_index_ == i)
			sz -= animation_id_ * 3;


		// More details and examples:
		// - https://doc.qt.io/qt-5.10/qpainter.html#drawPixmap

		// Target
		int pixmapTargetXposition = MARGIN * 2 + (64 - sz) / 2 + (i / ROWS) * 64;
		int pixmapTargetYposition = MARGIN *2 + j * 64 + TOP + (64 - sz) / 2;

		QPoint pixmapTarget(pixmapTargetXposition, pixmapTargetYposition);

		// Source
		int pixmapWidth = sz;
		int pixmapHeight = sz;

		QSize pixmapSize(pixmapWidth, pixmapHeight);

		// "The QPixmap class is an off-screen image representation that can be used as a paint device."
		// - https://doc.qt.io/qt-5.10/qpixmap.html
		// "Returns a pixmap with the requested size, mode, and state,"
		// - https://doc.qt.io/qt-5.10/qicon.html#pixmap
		QPixmap pixmap = icon.pixmap(pixmapSize, QIcon::Normal, QIcon::On);


		// Paint pixmap items
		painter.drawPixmap(pixmapTarget, pixmap);
	}


	// Close button
	// Rectangle size & coordinates for the close button
//	QRect closeButtonRectSize(MARGIN * 2 + cols * 64 - 8, 8, 12, 3);
	QRect closeButtonRectSize(MARGIN * 2 + cols * 64 - 14,6, 12, 3);

	// Color for the close button
	QBrush closeButtonRectColor(Qt::white);

	// Fills the given rectangle with the color
	painter.fillRect(closeButtonRectSize, closeButtonRectColor);


	// Default font
	painter.setFont(QFont("Sans", TOP, QFont::Normal));

	// Animation timer detay if animations are enabled
	if (animation_id_ > 0) {
		animation_id_--;
		QTimer::singleShot(ADELAY, this, SLOT(update()));
	}
}


// Window resize
void MainWindow::resizeEvent(QResizeEvent * /* event */) {
	int nelem = applist.count();
	int cols = nelem / ROWS + 1;

	// margins
	QRegion m1(0, 0, cols * 64 + MARGIN * 4, TOP + ROWS * 64 + MARGIN * 4);
	QRegion m2(MARGIN, MARGIN + TOP, cols * 64 + MARGIN * 2, ROWS * 64 + MARGIN * 2);
	QRegion m3(MARGIN * 2, MARGIN * 2 + TOP, cols * 64, ROWS * 64);

	QRegion all = m1.subtracted(m2);
	all = all.united(m3);

	setMask(all);
}


// Window size hint
QSize MainWindow::sizeHint() const {
	int nelem = applist.count();
	int cols = nelem / ROWS + 1;

	return QSize(64 * cols + MARGIN * 4, ROWS * 64 + MARGIN * 4 + TOP);
}


bool MainWindow::event(QEvent *event) {
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

		int index = applications_get_index(helpEvent->pos());
		if (index == -1) {
			int x = helpEvent->pos().x();
			int y = helpEvent->pos().y();
			int nelem = applist.count();
			int cols = nelem / ROWS + 1;

			if (x >= MARGIN * 2 + cols * 64 - 8 && x <= MARGIN * 2 + cols * 64 + 4 &&
			   y >= 4 && y <= 15) {
			   	QToolTip::showText(helpEvent->globalPos(), QString("Minimize"));
			   	return true;
			}
			else if (x >= 0 && x < 64 &&
				   y >= 4 && y <= 15) {
			   	QToolTip::showText(helpEvent->globalPos(), QString("Run tools"));
			   	return true;
			}

			else
				QToolTip::hideText();
		}
		else {
			QToolTip::showText(helpEvent->globalPos(), applist[index].description_);
			return true;
		}
	}
	return QWidget::event(event);
}

void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reason) {
	if (reason == QSystemTrayIcon::Context)
		return;
	if (reason == QSystemTrayIcon::DoubleClick)
		return;
	if (reason == QSystemTrayIcon::MiddleClick)
		return;

	if (isVisible()) {
		hide();
//		stats_->hide();
	}
	else {
		show();
//		stats_->hide();
	}
}


void MainWindow::createTrayActions() {
	minimizeAction = new QAction(tr("Mi&nimize"), this);
	connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
//	connect(minimizeAction, SIGNAL(triggered()), stats_, SLOT(hide()));

	restoreAction = new QAction(tr("&Restore"), this);
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));
//	connect(restoreAction, SIGNAL(triggered()), stats_, SLOT(show()));

	quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, SIGNAL(triggered()), this, SLOT(main_quit()));
}

void MainWindow::createLocalActions() {
	QAction *newsandbox = new QAction(tr("&Configuration wizard"), this);
	connect(newsandbox, SIGNAL(triggered()), this, SLOT(newSandbox()));
	addAction(newsandbox);

	QAction *runtools = new QAction(tr("&Tools"), this);
	connect(runtools, SIGNAL(triggered()), this, SLOT(runTools()));
	addAction(runtools);

	QAction *about = new QAction(tr("&About"), this);
	connect(about, SIGNAL(triggered()), this, SLOT(runAbout()));
	addAction(about);

	QAction *separator1 = new QAction(this);
	separator1->setSeparator(true);
	addAction(separator1);

//	qrun_ = new QAction(tr("&Run"), this);
//	connect(qrun_, SIGNAL(triggered()), this, SLOT(run()));
//	addAction(qrun_);

//	qedit_ = new QAction(tr("&Edit"), this);
//	connect(qedit_, SIGNAL(triggered()), this, SLOT(edit()));
//	addAction(qedit_);

//	qdelete_ = new QAction(tr("&Delete"), this);
//	connect(qdelete_, SIGNAL(triggered()), this, SLOT(remove()));
//	addAction(qdelete_);

	qhelp_ = new QAction(tr("&Help"), this);
	connect(qhelp_, SIGNAL(triggered()), this, SLOT(help()));
	addAction(qhelp_);

	QAction *separator2 = new QAction(this);
	separator2->setSeparator(true);
	addAction(separator2);

	QAction *qminimize = new QAction(tr("&Minimize"), this);
	connect(qminimize, SIGNAL(triggered()), this, SLOT(showMinimized()));
	addAction(qminimize);

	QAction *qquit = new QAction(tr("&Quit"), this);
	connect(qquit, SIGNAL(triggered()), this, SLOT(main_quit()));
	addAction(qquit);
}

// Help dialog
void MainWindow::help() {
	QMessageBox msgBox;

	QString txt;
	txt += "<br/>";
	txt += "Click on \"Firetools\" in the left top corner to open the tools window.<br/>\n";
	txt += "Click on \"-\" in the right top corner to minimize the launcher in the system tray.<br/>\n";
	txt += "<br/>";
	txt += "Double click on an icon to open an application.<br/>\n";
	txt += "Drag the launcher with the left mouse button.<br/>\n";
	txt += "Use the right mouse button to open a context menu.<br/>\n";
	txt += "<br/>";
	txt += "<b>Context Menu</b><br/><br/>\n";
	txt += "<b>Tools:</b> open the tools window<br/>\n";
	txt += "<b>New Sandbox:</b> start a new sandbox<br/>\n";
	txt += "<b>Minimize:</b> minimize the launcher<br/>\n";
	txt += "<b>Run:</b> start the program in a new sandbox.<br/>\n";
	txt += "<b>Edit:</b> edit the sandbox launcher.<br/>\n";
	txt += "<b>Delete:</b> delete the sandbox launcher.<br/>\n";
	txt += "<b>Help:</b> this help window.<br/>\n";
	txt += "<b>Quit:</b> shut down the lprogram.<br/>\n";
	txt += "<br/><br/>";

	QMessageBox::about(this, tr("Firejail Launcher"), txt);
}

// Shutdown sequence
void MainWindow::main_quit() {
	printf("exiting...\n");

	// delete application list
	QList<Application>::iterator it = applist.begin();
	while (it !=applist.end())
		it = applist.erase(it);

	qApp->quit();
}

