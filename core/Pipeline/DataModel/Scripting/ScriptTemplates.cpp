/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020–2026 Alex Spataru
 *
 * This file is dual-licensed:
 *
 * - Under the GNU GPLv3 (or later) for builds that exclude Pro modules.
 * - Under the Serial Studio Commercial License for builds that include
 *   any Pro functionality.
 *
 * You must comply with the terms of one of these licenses, depending
 * on your use case.
 *
 * For GPL terms, see <https://www.gnu.org/licenses/gpl-3.0.html>
 * For commercial terms, see LICENSES/LicenseRef-SerialStudio-Commercial.txt.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-SerialStudio-Commercial
 */

#include "DataModel/Scripting/ScriptTemplates.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Core/LanguageTable.h"
#include "Core/Services.h"
#include "Core/Translator.h"

/**
 * @brief Maps a Misc::Translator::Language enum to its IETF locale key.
 */
static QString localeKeyForLanguage(const Misc::Translator::Language language)
{
  return Misc::LanguageTable::qmName(language);
}

/**
 * @brief Reads a Qt resource file as UTF-8 text, or returns empty on failure.
 */
QString DataModel::readTextResource(const QString& path)
{
  QFile file(path);
  if (file.open(QFile::ReadOnly))
    return QString::fromUtf8(file.readAll());

  return {};
}

/**
 * @brief Picks the manifest's translation for the active UI language, falling back to Qt tr().
 */
QString DataModel::localizedTemplateName(const QJsonObject& object, const char* translationContext)
{
  auto& translator = Core::services().translator;

  QString fallback        = object.value(QStringLiteral("name")).toString();
  const auto translations = object.value(QStringLiteral("translations")).toObject();
  const QString localeKey = localeKeyForLanguage(translator.language());

  if (!localeKey.isEmpty()) {
    QString localized = translations.value(localeKey).toString();
    if (!localized.isEmpty())
      return localized;
  }

  if (translationContext && *translationContext && !fallback.isEmpty()) {
    const QByteArray utf8 = fallback.toUtf8();
    QString translated    = QCoreApplication::translate(translationContext, utf8.constData());
    if (!translated.isEmpty())
      return translated;
  }

  return fallback;
}

/**
 * @brief Parses a templates.json manifest into ScriptTemplateDefinition entries
 * (file/name/default).
 */
QList<DataModel::ScriptTemplateDefinition> DataModel::loadScriptTemplateManifest(
  const QString& manifestPath, const char* translationContext)
{
  QList<ScriptTemplateDefinition> templates;
  const auto json = readTextResource(manifestPath);
  if (json.isEmpty())
    return templates;

  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isArray())
    return templates;

  const auto array = document.array();
  templates.reserve(array.size());

  for (const auto& entry : array) {
    if (!entry.isObject())
      continue;

    const auto object = entry.toObject();
    const auto file   = object.value(QStringLiteral("file")).toString();
    if (file.isEmpty())
      continue;

    ScriptTemplateDefinition definition;
    definition.file      = file;
    definition.name      = localizedTemplateName(object, translationContext);
    definition.isDefault = object.value(QStringLiteral("default")).toBool(false);
    templates.append(definition);
  }

  return templates;
}

/**
 * @brief Returns the resource path "<directory>/<file><suffix>" for a template.
 */
QString DataModel::templateResourcePath(const QString& directory,
                                        const QString& file,
                                        const QString& suffix)
{
  return QStringLiteral("%1/%2%3").arg(directory, file, suffix);
}
