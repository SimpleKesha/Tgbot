namespace CS2BodycamInjector;

partial class MainForm
{
    private System.ComponentModel.IContainer? components = null;

    protected override void Dispose(bool disposing)
    {
        if (disposing && components != null) components.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        btnLaunchCS2 = new Button();
        btnInject    = new Button();
        listBoxLog   = new ListBox();
        lblTitle     = new Label();
        lblStatus    = new Label();
        panelTop     = new Panel();

        panelTop.SuspendLayout();
        SuspendLayout();

        // ── Цветовая палитра ──────────────────────────────────────────────
        var bg       = Color.FromArgb(18, 18, 18);
        var surface  = Color.FromArgb(28, 28, 30);
        var accent   = Color.FromArgb(0, 120, 212);   // синий — Launch
        var danger   = Color.FromArgb(200, 30, 30);   // красный — Inject
        var fg       = Color.FromArgb(220, 220, 220);
        var fgDim    = Color.FromArgb(100, 100, 100);
        var logBg    = Color.FromArgb(12, 12, 12);
        var logFg    = Color.FromArgb(0, 230, 100);

        // ── panelTop ─────────────────────────────────────────────────────
        panelTop.BackColor  = surface;
        panelTop.Dock       = DockStyle.Top;
        panelTop.Height     = 56;
        panelTop.Padding    = new Padding(12, 8, 12, 8);

        // ── lblTitle ─────────────────────────────────────────────────────
        lblTitle.AutoSize  = true;
        lblTitle.Text      = "CS2 BODYCAM MOD — INJECTOR";
        lblTitle.Font      = new Font("Segoe UI", 13F, FontStyle.Bold);
        lblTitle.ForeColor = fg;
        lblTitle.Location  = new Point(14, 14);

        // ── btnLaunchCS2 ─────────────────────────────────────────────────
        btnLaunchCS2.Location  = new Point(12, 68);
        btnLaunchCS2.Size      = new Size(218, 42);
        btnLaunchCS2.Text      = "▶  Launch CS2  (-insecure)";
        btnLaunchCS2.Font      = new Font("Segoe UI", 10F, FontStyle.Bold);
        btnLaunchCS2.BackColor = accent;
        btnLaunchCS2.ForeColor = Color.White;
        btnLaunchCS2.FlatStyle = FlatStyle.Flat;
        btnLaunchCS2.FlatAppearance.BorderSize = 0;
        btnLaunchCS2.Cursor    = Cursors.Hand;
        btnLaunchCS2.Click    += btnLaunchCS2_Click;

        // ── btnInject ────────────────────────────────────────────────────
        btnInject.Location  = new Point(240, 68);
        btnInject.Size      = new Size(218, 42);
        btnInject.Text      = "💉  Inject Bodycam Mod";
        btnInject.Font      = new Font("Segoe UI", 10F, FontStyle.Bold);
        btnInject.BackColor = danger;
        btnInject.ForeColor = Color.White;
        btnInject.FlatStyle = FlatStyle.Flat;
        btnInject.FlatAppearance.BorderSize = 0;
        btnInject.Cursor    = Cursors.Hand;
        btnInject.Click    += btnInject_Click;

        // ── listBoxLog ───────────────────────────────────────────────────
        listBoxLog.Location        = new Point(12, 122);
        listBoxLog.Size            = new Size(576, 280);
        listBoxLog.Font            = new Font("Consolas", 9F);
        listBoxLog.BackColor       = logBg;
        listBoxLog.ForeColor       = logFg;
        listBoxLog.BorderStyle     = BorderStyle.FixedSingle;
        listBoxLog.SelectionMode   = SelectionMode.None;
        listBoxLog.HorizontalScrollbar = true;

        // ── lblStatus ────────────────────────────────────────────────────
        lblStatus.Location  = new Point(14, 410);
        lblStatus.Size      = new Size(574, 18);
        lblStatus.Text      = "Source2 SDK  |  MinHook  |  CS2 Bodycam Mod v0.1-alpha";
        lblStatus.ForeColor = fgDim;
        lblStatus.Font      = new Font("Segoe UI", 8F);

        // ── MainForm ─────────────────────────────────────────────────────
        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode       = AutoScaleMode.Font;
        ClientSize          = new Size(600, 436);
        BackColor           = bg;
        Text                = "CS2 Bodycam Injector";
        FormBorderStyle     = FormBorderStyle.FixedSingle;
        MaximizeBox         = false;
        StartPosition       = FormStartPosition.CenterScreen;
        Icon                = SystemIcons.Shield;

        panelTop.Controls.Add(lblTitle);
        Controls.AddRange(new Control[]
        {
            panelTop, btnLaunchCS2, btnInject, listBoxLog, lblStatus
        });

        panelTop.ResumeLayout(false);
        panelTop.PerformLayout();
        ResumeLayout(false);
    }

    private Button  btnLaunchCS2 = null!;
    private Button  btnInject    = null!;
    private ListBox listBoxLog   = null!;
    private Label   lblTitle     = null!;
    private Label   lblStatus    = null!;
    private Panel   panelTop     = null!;
}
