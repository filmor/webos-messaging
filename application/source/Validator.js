enyo.kind({
    name: "Purple.AccountService",
    kind: "PalmService",
    service: "palm://org.webosinternals.libpurple.imaccountvalidator/"
});

enyo.kind({
    name: "Purple.Validator",
    kind: "VFlexBox",
    className: "basic-back",
    width: "100%",

    create: function() {
        this.inherited(arguments);

        this.$.getOptions.call({
            params: [{ prpl: "prpl-icq", locale: "de" }],
        });
    },

    gotOptions: function(inSender, inResponse) {
        this.$.options.setOptions(inResponse.options);
        this.render();
    },

    prefChanged: function() {
        console.log(arguments);
    },

    components: [
        {
            kind: "Toolbar",
            className: "enyo-toolbar-light accounts-header",
            pack: "center",
            components: [
                { kind: "Image", name: "icon" },
                { kind: "Control", name: "title", content: "Create Account" }
            ]
        },
        { className: "accounts-header-shadow" },
        {
            name: "validator",
            kind: "Scroller",
            flex: 1,
            components: [
                {
                    kind: "Control",
                    className: "box-center",
                    components: [
                        {
                            kind: "RowGroup",
                            caption: "USERNAME FROM TEMPLATE",
                            className: "accounts-group",
                            components: [
                                {
                                    name: "username",
                                    kind: "Input",
                                    inputType: "email",
                                    value: "",
                                    spellcheck: false,
                                    autoCapitalize: "lowercase",
                                    autocorrect: false
                                }
                            ]
                        },
                        {
                            kind: "RowGroup",
                            caption: $L("PASSWORD"),
                            className: "accounts-group",
                            components: [
                                {
                                    name: "password",
                                    kind: "Input",
                                    inputType: "password",
                                    value: "",
                                    spellcheck: false,
                                    autocorrect: false
                                }
                            ]
                        },
                        {
                            kind: "Purple.Options",
                            name: "options",
                            onPreferenceChanged: "prefChanged"
                        },
                        { name: "createButton", kind: "ActivityButton",
                          caption: $L("SIGN IN")
                        }
                    ]
                }
            ]
        },
        {
            name: "getOptions",
            kind: "Purple.AccountService",
            onSuccess: "gotOptions",
        },
        {
            name: "validateAccount",
            kind: "Purple.AccountService"
        }
    ]
});
