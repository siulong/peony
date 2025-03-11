/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#ifndef PEONYSEARCHVFSFILEENUMERATOR_H
#define PEONYSEARCHVFSFILEENUMERATOR_H

#include <gio/gio.h>
#include <QQueue>
#include <QRegExp>
#include <QObject>
#include <QEventLoop>
#include "file-info.h"
#ifdef KY_UKUI_SEARCH
#include "ukui-search/ukui-search-task.h"
#include "ukui-search/data-queue.h"
#include "ukui-search/result-item.h"
#endif

G_BEGIN_DECLS

#define PEONY_TYPE_SEARCH_VFS_FILE_ENUMERATOR peony_search_vfs_file_enumerator_get_type()
G_DECLARE_FINAL_TYPE(PeonySearchVFSFileEnumerator,
                     peony_search_vfs_file_enumerator,
                     PEONY, SEARCH_VFS_FILE_ENUMERATOR,
                     GFileEnumerator)

PeonySearchVFSFileEnumerator *peony_search_vfs_file_enumerator_new(void);

enum PeonySearchStatus {
    SEARCHFILE,
    SEARCHFILECONTENT,
    SEARCHFILEING,
    SEARCHFILECONTENTING
};

typedef struct {
    QString *search_vfs_directory_uri;
    /*!
     * \brief search_hidden
     * \deprecated
     */
    gboolean search_hidden;
    gboolean use_regexp;
    gboolean save_result;
    gboolean recursive;
    gboolean case_sensitive;
    QRegExp *name_regexp;
    QRegExp *content_regexp;
    QList<QRegExp*> *name_regexp_extend_list;
    gboolean match_name_or_content;
    QQueue<QString> *enumerate_queue;
#ifdef KY_UKUI_SEARCH
    UkuiSearch::UkuiSearchTask *m_search;
    UkuiSearch::DataQueue<UkuiSearch::ResultItem> *m_queue;
    UkuiSearch::UkuiSearchTask *m_contentSearch;
    UkuiSearch::DataQueue<UkuiSearch::ResultItem> *m_contentQueue;
    gboolean search_engine;
    PeonySearchStatus search_status;
    PeonySearchStatus search_content;
    QHash<QString, QString> *m_duplicatesHash;
#endif
} PeonySearchVFSFileEnumeratorPrivate;

struct _PeonySearchVFSFileEnumerator
{
    GFileEnumerator parent_instance;

    PeonySearchVFSFileEnumeratorPrivate *priv;
};

G_END_DECLS

#endif // PEONYSEARCHVFSFILEENUMERATOR_H
