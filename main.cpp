#include <QtWidgets/QtWidgets>

class DprGadget : public QWidget
{
  public:
  std::function<void()> m_updateFn;
  std::function<void()> m_clearFn;
  qreal m_currentDpr = -1;
  QString m_eventsText;

  DprGadget()
  {
    setWindowTitle(QString("DprGadget - Qt %1").arg(qVersion()));

    QFont tinyFont;   tinyFont.setPointSize(8);
    QFont smallFont;  smallFont.setPointSize(12);
    QFont bigFont;    bigFont.setPointSize(42);
    QFont biggerFont; biggerFont.setPointSize(80);

    auto makeLabel = [](const QString &text, const QFont &font, bool selectable = false) {
      auto *l = new QLabel(text);
      l->setFont(font);
      if (selectable)
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
      return l;
    };

    auto *dprLabel       = makeLabel("Device Pixel Ratio", bigFont);
    auto *dprValue       = makeLabel("", biggerFont, true);
    auto *screenLabel    = makeLabel("Current Screen:", smallFont, true);
    auto *sizeLabel      = makeLabel("QWindow size:", smallFont, true);
    auto *nativeSizeLabel= makeLabel("Native size:", smallFont, true);
    auto *windowDpiLabel = makeLabel("QWindow DPI:", smallFont, true);
    auto *windowDprLabel = makeLabel("QWindow DPR:", smallFont, true);
    auto *screenDprLabel = makeLabel("Screen DPR:", smallFont, true);
    auto *screenDpiLabel = makeLabel("Screen logical DPI:", smallFont, true);
    auto *eventsLabel    = makeLabel("", tinyFont, true);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(dprLabel);
    layout->setAlignment(dprLabel, Qt::AlignHCenter);
    layout->addWidget(dprValue);
    layout->setAlignment(dprValue, Qt::AlignHCenter);
    layout->addStretch();

    auto addRow = [&](QWidget *left, QWidget *right) {
      auto *row = new QHBoxLayout();
      row->addWidget(left);
      row->addStretch();
      row->addWidget(right);
      layout->addLayout(row);
    };

    auto addCentered = [&](QWidget *w) {
      auto *row = new QHBoxLayout();
      row->addStretch();
      row->addWidget(w);
      row->addStretch();
      layout->addLayout(row);
    };

    addCentered(screenLabel);
    addRow(sizeLabel, nativeSizeLabel);
    addRow(windowDpiLabel, screenDpiLabel);
    addRow(windowDprLabel, screenDprLabel);
    layout->addWidget(eventsLabel);

    // --- Active environment variables ---
    struct EnvVar { const char *name; const char *label; };
    const EnvVar envVars[] = {
                               { "QT_SCALE_FACTOR",                  "QT_SCALE_FACTOR" },
                               { "QT_USE_PHYSICAL_DPI",              "QT_USE_PHYSICAL_DPI" },
                               { "QT_FONT_DPI",                      "QT_FONT_DPI" },
                               { "QT_SCALE_FACTOR_ROUNDING_POLICY",  "QT_SCALE_FACTOR_ROUNDING_POLICY" },
                               };
    bool anyEnv = false;
    for (auto &ev : envVars)
      if (qEnvironmentVariableIsSet(ev.name)) { anyEnv = true; break; }

    if (anyEnv) {
      layout->addWidget(new QLabel("Active Environment:"));
      for (auto &ev : envVars) {
        if (qEnvironmentVariableIsSet(ev.name)) {
          layout->addWidget(new QLabel(
              QString("%1=%2").arg(ev.label, qgetenv(ev.name))));
        }
      }
    }

    // --- Update function (public APIs only) ---
    auto updateValues = [=]() {
      QScreen *scr = screen();

      // Window DPR (the one Qt exposes to the app)
      dprValue->setText(QString::number(devicePixelRatioF(), 'f', 2));

      // QWindow size (logical)
      sizeLabel->setText(QString("QWindow size: %1 × %2")
                             .arg(width()).arg(height()));

      // Native (physical) size = logical × DPR
      if (windowHandle()) {
        QSize native = windowHandle()->size() * devicePixelRatioF();
        nativeSizeLabel->setText(QString("Native: %1 × %2")
                                     .arg(native.width()).arg(native.height()));

        // QWindow DPR
        windowDprLabel->setText(QString("QWindow DPR: %1")
                                    .arg(windowHandle()->devicePixelRatio(), 0, 'f', 2));
      }

      // QWindow logical DPI
      windowDpiLabel->setText(QString("QWindow DPI: %1")
                                  .arg(logicalDpiX()));

      if (scr) {
        // Screen DPR
        screenDprLabel->setText(QString("Screen DPR: %1")
                                    .arg(scr->devicePixelRatio(), 0, 'f', 2));

        // Screen logical DPI
        screenDpiLabel->setText(QString("Screen logical DPI: %1")
                                    .arg(scr->logicalDotsPerInch(), 0, 'f', 1));

        screenLabel->setText(QString("Current Screen: %1").arg(scr->name()));
      }

      eventsLabel->setText(m_eventsText);
    };

    m_updateFn = updateValues;
    m_clearFn  = [=]() {
      dprValue->setText("");
      m_eventsText.clear();
    };

    // Connect after the window handle exists
    create();

    QObject::connect(windowHandle(), &QWindow::screenChanged,
                     [this, updateValues](QScreen *) {
                       m_eventsText.prepend("ScreenChange ");
                       m_eventsText.truncate(120);
                       updateValues();
                     });

    setLayout(layout);
    updateValues();
  }

  protected:
  void paintEvent(QPaintEvent *) override
  {
    m_eventsText.prepend("Paint ");
    m_eventsText.truncate(120);

    if (m_currentDpr == devicePixelRatioF())
      return;
    m_currentDpr = devicePixelRatioF();
    m_updateFn();
  }

  void resizeEvent(QResizeEvent *event) override
  {
    QSize s = event->size();
    m_eventsText.prepend(QString("Resize(%1×%2) ").arg(s.width()).arg(s.height()));
    m_eventsText.truncate(120);
    m_updateFn();
  }

  void mousePressEvent(QMouseEvent *) override
  {
    m_clearFn();
    QTimer::singleShot(500, this, [this]() { m_updateFn(); });
  }
};

int main(int argc, char **argv)
{
  // Qt 6: high-DPI scaling is on by default; PassThrough gives exact factor
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QApplication app(argc, argv);

  DprGadget gadget;
  gadget.show();

  return app.exec();
}