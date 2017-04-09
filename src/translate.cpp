#include "mainwindow.h"

#include <Polygon4/DataManager/Common.h>
#include <Polygon4/DataManager/Database.h>
#include <Polygon4/DataManager/Storage.h>
#include <Polygon4/DataManager/StorageImpl.h>
#include <Polygon4/DataManager/TreeItem.h>
#include <Polygon4/DataManager/Types.h>

#include <primitives/executor.h>
#include <primitives/http.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <qmessagebox.h>
#include <qprogressdialog.h>
#include <qapplication.h>
#include <qinputdialog.h>

namespace pt = boost::property_tree;
using ptree = pt::ptree;

std::string ptree2string(const ptree &p)
{
    std::ostringstream oss;
    pt::write_json(oss, p, false);
    return oss.str();
}

ptree string2ptree(const std::string &s)
{
    ptree p;
    if (s.empty())
        return p;
    std::istringstream iss(s);
    pt::read_json(iss, p);
    return p;
}

String charToHex(char c)
{
    std::string result;
    char first, second;

    first = (c & 0xF0) / 16;
    first += first > 9 ? 'A' - 10 : '0';
    second = c & 0x0F;
    second += second > 9 ? 'A' - 10 : '0';

    result.append(1, first);
    result.append(1, second);

    return result;
}

String form_urlencode(const String &src)
{
    std::string result;
    for (auto &c : src)
    {
        /*if (isalnum(c))
        {
            result.append(1, c);
            continue;
        }*/
        switch (c)
        {
        case ' ':
            result.append(1, '+');
            break;
        case '-': case '_': case '.': case '!': case '~': case '*': case '\'':
        case '(': case ')':
            result.append(1, c);
            break;
            // escape
        default:
            result.append(1, '%');
            result.append(charToHex(c));
            break;
        }
    }
    return result;
}

struct Translator
{
    enum Type
    {
        Yandex,
        Google
    };

    Type type = Yandex;
    String key;
    String from;
    String to;

    bool translate(const String &from, String &to, String *error = nullptr) const
    {
        switch (type)
        {
        case Yandex:
            return yandex(from, to, error);
        case Google:
            return google(from, to, error);
        }
    }

private:
    bool yandex(const String &from, String &to, String *error) const
    {
        if (from.empty())
            return;
        if (!to.empty())
            return;

        HttpRequest req = httpSettings;
        req.url = "https://translate.yandex.net/api/v1.5/tr.json/translate"s + "?";
        req.url += "key=" + key + "&";
        req.url += "lang=" + from + "-" + to + "&";
        req.url += "text=" + form_urlencode(from);
        auto resp = url_request(req);
        auto p = string2ptree(resp.response);
        if (resp.http_code != 200)
        {
            if (error)
                *error = p.get("message", ""s);
            return false;
        }
        auto t = p.get_child("text");
        to = t.begin()->second.get_value<String>();
        return true;
    }

    bool google(const String &from, String &to, String *error) const
    {
        if (from.empty())
            return;
        if (!to.empty())
            return;

        HttpRequest req = httpSettings;
        req.url = "https://www.googleapis.com/language/translate/v2"s + "?";
        req.url += "key=" + key + "&";
        req.url += "source=" + from + "&";
        req.url += "target=" + to + "&";
        req.url += "q=" + form_urlencode(from);
        auto resp = url_request(req);
        auto p = string2ptree(resp.response);
        if (resp.http_code != 200)
        {
            if (error)
                *error = p.get("error.message", ""s);
            return false;
        }
        auto t = p.get_child("text");
        to = t.begin()->second.get_value<String>();
        return true;
    }
};

void MainWindow::translateStrings()
{
    if (!storage)
        return;

    std::atomic_int counter = 0;
    auto max = storage->strings.size() * (int)polygon4::LocalizationType::max;

    const auto key = QInputDialog::getText(this, windowTitle(), tr("Enter translate service key"));

    QProgressDialog progress(tr("Translating Strings..."), "Abort", 0, max, this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    progress.setFixedWidth(400);
    progress.setFixedHeight(75);
    progress.setCancelButton(0);
    progress.setValue(0);
    progress.setWindowModality(Qt::WindowModal);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    Executor e(10, "translator");
    auto stopped = false;
    String error;

    const String url = u8"https://translate.yandex.net/api/v1.5/tr.json/translate?key=" + key.toStdString() +"&";
    for (auto &s : storage->strings)
    {
#define ADD_LANGUAGE(language, id)                                                               \
    e.push([&s = s, &url, &counter, this, &stopped, &error ] {                                   \
        counter++;                                                                               \
                                                                                                 \
        if (s.second->string.ru.empty())                                                         \
            return;                                                                              \
        if (!s.second->string.language.empty())                                                  \
            return;                                                                              \
        if (stopped)                                                                             \
            return;                                                                              \
                                                                                                 \
        HttpRequest req = httpSettings;                                                          \
        req.type = HttpRequest::POST;                                                            \
        req.url = url + "lang=ru-" + #language + "&text=" + form_urlencode(s.second->string.ru); \
        auto resp = url_request(req);                                                            \
        auto p = string2ptree(resp.response);                                                    \
        if (resp.http_code != 200)                                                               \
        {                                                                                        \
            auto e = p.get("message", ""s);                                                      \
            error = e;                                                                           \
            stopped = true;                                                                      \
            return;                                                                              \
        }                                                                                        \
                                                                                                 \
        auto t = p.get_child("text");                                                            \
        auto tr = t.begin()->second.get_value<String>();                                         \
        s.second->string.language = tr;                                                          \
    });
#include <Polygon4/DataManager/Languages.inl>
    }

    while (counter < max)
    {
        progress.setValue(counter);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (stopped)
        {
            progress.setWindowModality(Qt::NonModal);
            progress.hide();
            QMessageBox::information(this, tr("Translation error"), error.c_str());
            break;
        }
    }

    e.wait();

    //saveDb();
}
