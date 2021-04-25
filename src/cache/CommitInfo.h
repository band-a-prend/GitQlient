#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QDateTime>
#include <QStringList>
#include <QVector>

#include <Lane.h>
#include <References.h>

class CommitInfo
{
public:
   enum class Field
   {
      SHA,
      PARENTS_SHA,
      COMMITER,
      AUTHOR,
      DATE,
      SHORT_LOG,
      LONG_LOG
   };

   CommitInfo() = default;
   ~CommitInfo();
   explicit CommitInfo(const QString sha, const QStringList &parents, const QDateTime &commitDate, const QString &log);
   explicit CommitInfo(const QString sha, const QStringList &parents, const QString &commiter,
                       const QDateTime &commitDate, const QString &author, const QString &log,
                       const QString &longLog = QString(), bool isSigned = false, const QString &gpgKey = QString());
   bool operator==(const CommitInfo &commit) const;
   bool operator!=(const CommitInfo &commit) const;

   bool contains(const QString &value);

   int parentsCount() const;
   QString parent(int idx) const;
   QStringList parents() const;
   bool isInWorkingBranch() const;

   QString sha() const { return mSha; }
   QString committer() const { return mCommitter; }
   QString author() const { return mAuthor; }
   QString authorDate() const { return QString::number(mCommitDate.toSecsSinceEpoch()); }
   QString shortLog() const { return mShortLog; }
   QString longLog() const { return mLongLog; }
   QString fullLog() const { return QString("%1\n\n%2").arg(mShortLog, mLongLog.trimmed()); }

   bool isValid() const;
   bool isWip() const { return mSha == ZERO_SHA; }

   void setLanes(const QVector<Lane> &lanes) { mLanes = lanes; }
   QVector<Lane> getLanes() const { return mLanes; }
   Lane getLane(int i) const { return mLanes.at(i); }
   int getLanesCount() const { return mLanes.count(); }
   int getActiveLane() const;

   void addChildReference(CommitInfo *commit) { mChilds.append(commit); }
   bool hasChilds() const { return !mChilds.empty(); }
   QString getFirstChildSha() const;
   int getChildsCount() const { return mChilds.count(); }

   bool isSigned() const { return mSigned; }
   QString getGpgKey() const { return mGpgKey; }

   void setPos(uint pos) { mPos = pos; }
   uint getPos() const { return mPos; }

   static const QString ZERO_SHA;
   static const QString INIT_SHA;

private:
   QString mSha;
   QStringList mParentsSha;
   QString mCommitter;
   QString mAuthor;
   QString mShortLog;
   QString mLongLog;
   QString mDiff;
   QDateTime mCommitDate;
   QVector<Lane> mLanes;
   QVector<CommitInfo *> mChilds;
   bool mSigned = false;
   QString mGpgKey;
   uint mPos = 0;
};
