/*
    SPDX-FileCopyrightText: 2006-2008 Robert Knight <robertknight@gmail.com>
    SPDX-FileCopyrightText: 2009 Thomas Dreibholz <dreibh@iem.uni-due.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "SaveHistoryTask.h"

#include <QApplication>
#include <QFileDialog>
#include <QTextStream>

#include <KConfig>
#include <KConfigGroup>
#include <KIO/TransferJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

#include "Emulation.h"
#include "session/SessionManager.h"

#include "../decoders/HTMLDecoder.h"
#include "../decoders/PlainTextDecoder.h"
#include "colorscheme/ColorScheme.h"
#include "colorscheme/ColorSchemeManager.h"

namespace Konsole
{
QString SaveHistoryTask::_saveDialogRecentURL;

SaveHistoryTask::SaveHistoryTask(QObject *parent)
    : SessionTask(parent)
{
}

SaveHistoryTask::~SaveHistoryTask() = default;

void SaveHistoryTask::execute()
{
    // TODO - think about the UI when saving multiple history sessions, if there are more than two or
    //        three then providing a URL for each one will be tedious

    // TODO - show a warning ( preferably passive ) if saving the history output fails
    QFileDialog *dialog = new QFileDialog(QApplication::activeWindow());
    dialog->setAcceptMode(QFileDialog::AcceptSave);

    QStringList mimeTypes{QStringLiteral("text/plain"), QStringLiteral("text/html")};
    dialog->setMimeTypeFilters(mimeTypes);

    KSharedConfigPtr konsoleConfig = KSharedConfig::openConfig();
    auto group = konsoleConfig->group(QStringLiteral("SaveHistory Settings"));

    if (_saveDialogRecentURL.isEmpty()) {
        const auto list = group.readPathEntry("Recent URLs", QStringList());
        if (list.isEmpty()) {
            dialog->setDirectory(QDir::homePath());
        } else {
            dialog->setDirectoryUrl(QUrl(list.at(0)));
        }
    } else {
        dialog->setDirectoryUrl(QUrl(_saveDialogRecentURL));
    }

    // iterate over each session in the task and display a dialog to allow the user to choose where
    // to save that session's history.
    // then start a KIO job to transfer the data from the history to the chosen URL
    const QList<QPointer<Session>> sessionsList = sessions();
    for (const auto &session : sessionsList) {
        dialog->setWindowTitle(i18n("Save Output From %1", session->title(Session::NameRole)));

        int result = dialog->exec();

        if (result != QDialog::Accepted) {
            continue;
        }

        QUrl url = (dialog->selectedUrls()).at(0);

        if (!url.isValid()) {
            // UI:  Can we make this friendlier?
            KMessageBox::error(nullptr, i18n("%1 is an invalid URL, the output could not be saved.", url.url()));
            continue;
        }

        // Save selected URL for next time
        _saveDialogRecentURL = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toString();
        group.writePathEntry("Recent URLs", _saveDialogRecentURL);

        KIO::TransferJob *job = KIO::put(url,
                                         -1, // no special permissions
                                         // overwrite existing files
                                         // do not resume an existing transfer
                                         // show progress information only for remote
                                         // URLs
                                         KIO::Overwrite | (url.isLocalFile() ? KIO::HideProgressInfo : KIO::DefaultFlags)
                                         // a better solution would be to show progress
                                         // information after a certain period of time
                                         // instead, since the overall speed of transfer
                                         // depends on factors other than just the protocol
                                         // used
        );

        SaveJob jobInfo;
        jobInfo.session = session;
        jobInfo.lastLineFetched = -1; // when each request for data comes in from the KIO subsystem
        // lastLineFetched is used to keep track of how much of the history
        // has already been sent, and where the next request should continue
        // from.
        // this is set to -1 to indicate the job has just been started

        if (((dialog->selectedNameFilter()).contains(QLatin1String("html"), Qt::CaseInsensitive))
            || ((dialog->selectedFiles()).at(0).endsWith(QLatin1String("html"), Qt::CaseInsensitive))) {
            Profile::Ptr profile = SessionManager::instance()->sessionProfile(session);
            const auto schemeName = profile->colorScheme();
            const auto scheme = ColorSchemeManager::instance()->findColorScheme(schemeName);
            QColor colorTable[TABLE_COLORS];
            if (scheme) {
                scheme->getColorTable(colorTable);
            } else {
                std::copy_n(ColorScheme::defaultTable, TABLE_COLORS, colorTable);
            }

            jobInfo.decoder = new HTMLDecoder(colorTable);
        } else {
            jobInfo.decoder = new PlainTextDecoder();
        }

        _jobSession.insert(job, jobInfo);

        connect(job, &KIO::TransferJob::dataReq, this, &Konsole::SaveHistoryTask::jobDataRequested);
        connect(job, &KIO::TransferJob::result, this, &Konsole::SaveHistoryTask::jobResult);
    }

    dialog->deleteLater();
}

void SaveHistoryTask::jobDataRequested(KIO::Job *job, QByteArray &data)
{
    // TODO - Report progress information for the job

    // PERFORMANCE:  Do some tests and tweak this value to get faster saving
    const int LINES_PER_REQUEST = 500;

    SaveJob &info = _jobSession[job];

    // transfer LINES_PER_REQUEST lines from the session's history
    // to the save location
    if (!info.session.isNull()) {
        // note:  when retrieving lines from the emulation,
        // the first line is at index 0.

        int sessionLines = info.session->emulation()->lineCount();

        if (sessionLines - 1 == info.lastLineFetched) {
            return; // if there is no more data to transfer then stop the job
        }

        int copyUpToLine = qMin(info.lastLineFetched + LINES_PER_REQUEST, sessionLines - 1);

        QTextStream stream(&data, QIODevice::ReadWrite);
        info.decoder->begin(&stream);
        info.session->emulation()->writeToStream(info.decoder, info.lastLineFetched + 1, copyUpToLine);
        info.decoder->end();

        info.lastLineFetched = copyUpToLine;
    }
}
void SaveHistoryTask::jobResult(KJob *job)
{
    if (job->error() != 0) {
        KMessageBox::error(nullptr, i18n("A problem occurred when saving the output.\n%1", job->errorString()));
    }

    TerminalCharacterDecoder *decoder = _jobSession[job].decoder;

    _jobSession.remove(job);

    delete decoder;

    // notify the world that the task is done
    Q_EMIT completed(true);

    if (autoDelete()) {
        deleteLater();
    }
}

}

#include "moc_SaveHistoryTask.cpp"
