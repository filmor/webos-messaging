
enyo.kind({
    name: "Purple.Options",
    kind: "Control",

    events: {
        onPreferenceChanged: ""
    },

    published: {
        options: {}
    },

    setOptions: function(options) {
        if (this.$._group)
            this.$._group.destroy();

        this.createComponent({
            name: "_group",
            kind: "RowGroup",
            caption: $L("Advanced Options")
        })

        this.$.options = options;
        console.log(options);
        for (var name in options)
        {
            var node = options[name];

            var innerComponent = new Object();

            if (node.type == "string")
            {
                innerComponent = {
                    kind: "Input",
                    hint: node.type,
                    autocorrect: false,
                    spellcheck: false,
                    value: node.default_value,
                    onchange: "onChange"
                }
            }
            else if (node.type == "int")
            {
                innerComponent = {
                    kind: "Input",
                    hint: node.type,
                    autocorrect: false,
                    spellcheck: false,
                    value: node.default_value,
                    onchange: "onIntInput",
                    onkeypress: "filterInt"
                };
            }
            else if (node.type == "bool")
            {
                innerComponent = {
                    kind: "CheckBox",
                    checked: node.default_value,
                    onChange: "onCheck"
                };
            }
            else if (node.type == "list")
            {
                var items = [];
                for (var name_ in node.choices)
                    items.push({caption: node.choices[name_],
                                value: name_
                                });

                console.log(items);

                innerComponent = {
                    kind: "ListSelector",
                    items: items,
                    onChange: "onChange"
                };
            }
            else
            {
                // TODO: ERROR!
            }

            innerComponent["name"] = name;
            console.log(innerComponent);

            this.$._group.createComponent({
                layoutKind: "HFlexLayout",
                components: [
                    {content: node.text, flex: 1},
                    innerComponent
                ]
            });
        }
    },

    filterInt: function(inSender, inEvent) {
        var key = inEvent.keycode || inEvent.which;
        if (key == 13) // Enter
            return;

        key = String.fromCharCode(key);
        var regex = /[0-9]/;
        console.log(key);
        if (!regex.test(key)) {
            inEvent.preventDefault();
        }
    },

    onIntInput: function(inSender) {
        var val = Number(inSender.getValue());
        console.log(val);

        if (val != NaN)
            this.doPreferenceChanged(inSender.name, val);
        else
            return;
        // TODO
    },

    onChange: function(inSender) {
        this.doPreferenceChanged(inSender.name, inSender.getValue());
    },

    onCheck: function(inSender) {
        this.doPreferenceChanged(inSender.name, inSender.getChecked());
    }
})
