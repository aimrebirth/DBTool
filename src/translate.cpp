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
    String source;
    String target;

    bool translate(const polygon4::String &from, polygon4::String &to, String *error = nullptr) const
    {
        if (from.empty())
            return true;
        if (!to.empty())
            return true;

        switch (type)
        {
        case Yandex:
            return yandex(from, to, error);
        case Google:
            return google(from, to, error);
        }
        return false;
    }

private:
    bool yandex(const polygon4::String &from, polygon4::String &to, String *error) const
    {
        HttpRequest req = httpSettings;
        req.url = "https://translate.yandex.net/api/v1.5/tr.json/translate"s + "?";
        req.url += "key=" + key + "&";
        req.url += "lang=" + source + "-" + target + "&";
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

    bool google(const polygon4::String &from, polygon4::String &to, String *error) const
    {
        HttpRequest req = httpSettings;
        req.url = "https://www.googleapis.com/language/translate/v2"s + "?";
        req.url += "key=" + key + "&";
        req.url += "source=" + source + "&";
        req.url += "target=" + target + "&";
        req.url += "q=" + form_urlencode(from);
        auto resp = url_request(req);
        auto p = string2ptree(resp.response);
        if (resp.http_code != 200)
        {
            if (error)
                *error = p.get("error.message", ""s);
            return false;
        }
        auto t = p.get_child("data.translations");
        to = t.begin()->second.get<String>("translatedText", "");
        if (iswupper(from.c_str()[0]))
            ((wchar_t*)to.c_str())[0] = towupper(to.c_str()[0]);
        return true;
    }
};

void MainWindow::translateStrings()
{
    if (!storage)
        return;

    Translator td;
    td.type = Translator::Google;
    td.key = QInputDialog::getText(this, windowTitle(), tr("Enter translate service key")).toStdString();

    Executor e(1, "translator");
    auto stopped = false;
    String error;
    std::atomic_size_t counter = 0;
    size_t max = 0;

    auto p4tr = [&, &td = td](polygon4::String polygon4::LocalizedString::*from, polygon4::String polygon4::LocalizedString::*to)
    {
        for (auto &s : storage->strings)
        {
            e.push([&s = s, &stopped, &error, td, &counter, from, to]
            {
                counter++;
                if (stopped)
                    return;
                if (!td.translate(s.second->string.*from, s.second->string.*to, &error))
                    stopped = true;
            });
            max++;
            break;
        }
    };

    QProgressDialog progress(tr("Translating Strings..."), "Abort", 0, max, this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    progress.setFixedWidth(400);
    progress.setFixedHeight(75);
    progress.setCancelButton(0);
    progress.setValue(0);
    progress.setWindowModality(Qt::WindowModal);
    QApplication::processEvents();

    auto ui_wait = [&]()
    {
        while (counter < max)
        {
            progress.setValue(counter);
            QApplication::processEvents();
            if (stopped)
                break;
        }
    };

    td.source = "ru";
    td.target = "en";
    p4tr(&polygon4::LocalizedString::ru, &polygon4::LocalizedString::en);

    progress.setMaximum(max);
    ui_wait();
    e.wait(); // we must fully proceed english strings first

    td.source = "en";
#define ADD_TRANSLATION(x) td.target = #x; p4tr(&polygon4::LocalizedString::en, &polygon4::LocalizedString::x)
    ADD_TRANSLATION(de);
    ADD_TRANSLATION(fr);
    ADD_TRANSLATION(es);
    ADD_TRANSLATION(et);
    ADD_TRANSLATION(cs);
    ui_wait();
    e.wait();

    progress.hide();
    QApplication::processEvents();
    if (stopped)
        QMessageBox::information(this, tr("Translation error"), error.c_str());

    //saveDb();
}
