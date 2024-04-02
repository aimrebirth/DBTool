#pragma sw require header org.sw.demo.qtproject.qt.base.tools.moc

void build(Solution &s)
{
    auto &DBTool = s.addExecutable("Polygon4.DBTool", "master");
    DBTool += Git("https://github.com/aimrebirth/DBTool", "", "{v}");

    DBTool += cpp23;
    if (auto L = DBTool.getSelectedTool()->as<VisualStudioLinker*>(); L)
        L->Subsystem = vs::Subsystem::Windows;

    DBTool += "res/.*"_rr;
    DBTool += "src/.*"_rr;
    DBTool += "USE_QT"_def;

    DBTool += "pub.lzwdgc.Polygon4.DataManager-master"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.widgets"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.winmain"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.plugins.platforms.windows"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.plugins.styles.modernwindows"_dep;

    DBTool += "pub.egorpugin.primitives.http"_dep;
    DBTool += "pub.egorpugin.primitives.executor"_dep;

    qt_moc_rcc_uic("org.sw.demo.qtproject.qt"_dep, DBTool);
    qt_tr("org.sw.demo.qtproject.qt"_dep, DBTool);

    RccData d;
    d.prefix = "/ts";
    d.name = "qm";
    for (auto &[p, f] : DBTool)
    {
        if (f->skip)
            continue;
        if (p.extension() == ".qm")
            d.files[p];
    }
    rcc("org.sw.demo.qtproject.qt.base.tools.rcc"_dep, DBTool, d);

    {
        auto c = DBTool.addCommand();
        c << cmd::prog("git"s)
            << "rev-list" << "HEAD" << "--count"
            << cmd::std_out("version_gen.h");
    }

    {
        auto c = DBTool.addCommand(SW_VISIBLE_BUILTIN_FUNCTION(copy_file));
        c << cmd::in("version_gen.h") << cmd::out("version.h.in");
    }
}
