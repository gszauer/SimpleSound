class WasmMath {
    constructor(wasmImportObject) {
        if (!wasmImportObject.hasOwnProperty("env")) {
            wasmImportObject.env = {};
        }
        let self = this;

        wasmImportObject.env["MathCos"] = function(x) {
            return self.Cos(x);
        };

        wasmImportObject.env["MathSin"] = function(x) {
            return self.Sin(x);
        };

        wasmImportObject.env["MathSqrt"] = function(x) {
            return self.Sqrt(x);
        };
    }

    Cos(x) {
        return Math.cos(x);
    }

    Sin(x) {
        return Math.sin(x);
    }

    Sqrt(x) {
        return Math.sqrt(x);
    }
}