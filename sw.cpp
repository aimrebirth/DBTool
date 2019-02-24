#pragma sw require header org.sw.demo.qtproject.qt.base.tools.moc-*
#pragma sw require header org.sw.demo.qtproject.qt.tools.linguist.release-*

void build(Solution &s)
{
    auto &DBTool = s.addExecutable("Polygon4.DBTool", "master");
    DBTool += Git("https://github.com/aimrebirth/DBTool", "", "{v}");

    DBTool.CPPVersion = CPPLanguageStandard::CPP17;
    if (auto L = DBTool.getSelectedTool()->as<VisualStudioLinker>(); L)
        L->Subsystem = vs::Subsystem::Windows;

    DBTool += "res/.*"_rr;
    DBTool += "src/.*"_rr;
    DBTool += "USE_QT"_def;

    DBTool += "pub.lzwdgc.Polygon4.DataManager-master"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.widgets-5"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.winmain-5"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.plugins.platforms.windows-5"_dep;
    DBTool += "org.sw.demo.qtproject.qt.base.plugins.styles.windowsvista-5"_dep;

    DBTool += "pub.egorpugin.primitives.http-master"_dep;
    DBTool += "pub.egorpugin.primitives.executor-master"_dep;

    /*automoc("org.sw.demo.qtproject.qt.base.tools.moc-*"_dep, DBTool);
    rcc("org.sw.demo.qtproject.qt.base.tools.rcc-*"_dep, t, t.SourceDir / ("resources/" + t.pkg.ppath.back() + ".qrc"));
    qt_tr("org.sw.demo.qtproject.qt-*"_dep, t);*/

    qt_moc_rcc_uic("org.sw.demo.qtproject.qt-*"_dep, DBTool);
    qt_tr("org.sw.demo.qtproject.qt-*"_dep, DBTool);

    RccData d;
    d.prefix = "/ts";
    d.name = "qm";
    for (auto &[p, f] : DBTool)
    {
        if (f->skip)
            continue;
        auto ext = p.extension().u8string();
        if (ext == ".qm")
            d.files[p];
    }
    rcc("org.sw.demo.qtproject.qt.base.tools.rcc-*"_dep, DBTool, d);

    {
        auto c = DBTool.addCommand();
        c << "git"
            << "rev-list" << "HEAD" << "--count"
            << cmd::std_out("version_gen.h");

        SW_MAKE_EXECUTE_BUILTIN_COMMAND_AND_ADD(c2, DBTool, "sw_copy_file");
        c2->args.push_back(normalize_path(DBTool.BinaryDir / "version_gen.h"));
        c2->args.push_back(normalize_path(DBTool.BinaryDir / "version.h.in"));
        c2->addInput(DBTool.BinaryDir / "version_gen.h");
        c2->addOutput(DBTool.BinaryDir / "version.h.in");
        DBTool += "version.h.in";
    }
}