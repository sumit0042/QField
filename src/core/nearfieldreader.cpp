/***************************************************************************
 nearfieldreader.cpp - NearFieldReader

 ---------------------
 begin                : 27.08.2023
 copyright            : (C) 2023 by Mathieu Pellerin
 email                : mathieu (at) opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "nearfieldreader.h"

#include <QDebug>
#include <QNdefMessage>
#include <QNdefNfcTextRecord>
#include <QNearFieldTarget>
#include <QUrl>
#include <qgsmessagelog.h>

NearFieldReader::NearFieldReader( QObject *parent )
  : QObject( parent )
{
  mNearFieldManager = std::make_unique<QNearFieldManager>( this );
  connect( mNearFieldManager.get(), &QNearFieldManager::targetDetected, this, &NearFieldReader::handleTargetDetected );
  connect( mNearFieldManager.get(), &QNearFieldManager::targetDetected, this, &NearFieldReader::handleTargetLost );
}

void NearFieldReader::handleTargetDetected( QNearFieldTarget *target )
{
  connect( target, &QNearFieldTarget::ndefMessageRead, this, &NearFieldReader::handleNdefMessageRead );
  connect( target, &QNearFieldTarget::error, this, &NearFieldReader::handleTargetError );

  emit targetDetected( QString( target->uid() ) );

  if ( target->hasNdefMessage() )
  {
    mReadString.clear();
    emit readStringChanged();

    QNearFieldTarget::RequestId request = target->readNdefMessages();
    if ( !request.isValid() )
    {
      handleTargetError( QNearFieldTarget::NdefReadError, request );
    }
  }
}

void NearFieldReader::handleTargetLost( QNearFieldTarget *target )
{
  disconnect( target );
}

void NearFieldReader::handleNdefMessageRead( const QNdefMessage &message )
{
  for ( const QNdefRecord &record : message )
  {
    QgsMessageLog::logMessage( record.type() );
    if ( record.isRecordType<QNdefNfcTextRecord>() )
    {
      QNdefNfcTextRecord textRecord( record );
      mReadString.append( textRecord.text() );
      emit readStringChanged();
    }
  }
}

void NearFieldReader::handleTargetError( QNearFieldTarget::Error error, const QNearFieldTarget::RequestId &id )
{
  qInfo() << QStringLiteral( "NFC error: %1" ).arg( error );
}

QString NearFieldReader::readString() const
{
  return mReadString;
}

bool NearFieldReader::active() const
{
  return mActive;
}

void NearFieldReader::setActive( bool active )
{
  if ( mActive == active )
    return;

  mActive = active;
  emit activeChanged();

  if ( mActive )
  {
    mReadString.clear();
    emit readStringChanged();

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    mNearFieldManager->startTargetDetection( QNearFieldTarget::NdefAccess );
#else
    mNearFieldManager->setTargetAccessModes( QNearFieldManager::NdefReadTargetAccess );
    mNearFieldManager->startTargetDetection();
#endif
  }
  else
  {
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    mNearFieldManager->setTargetAccessModes( QNearFieldManager::NoTargetAccess );
#endif
    mNearFieldManager->stopTargetDetection();
  }
}
